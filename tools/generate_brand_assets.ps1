param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$resourcesDir = Join-Path $RepositoryRoot 'resources'
$assetsDir = Join-Path $RepositoryRoot 'packaging/msix/Assets'

function New-InkdotPngBytes {
    param([Parameter(Mandatory)][int]$Size)

    $scale = 4
    $canvasSize = $Size * $scale
    $bitmap = [System.Drawing.Bitmap]::new(
        $canvasSize,
        $canvasSize,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

        $s = [single]$canvasSize
        $margin = $s * 0.065
        $paper = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 247, 241, 226))
        $paperEdge = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 216, 204, 180), $s * 0.018)
        try {
            $graphics.FillEllipse($paper, $margin, $margin, $s - 2 * $margin, $s - 2 * $margin)
            $graphics.DrawEllipse($paperEdge, $margin, $margin, $s - 2 * $margin, $s - 2 * $margin)
        }
        finally {
            $paper.Dispose()
            $paperEdge.Dispose()
        }

        $drop = [System.Drawing.Drawing2D.GraphicsPath]::new()
        try {
            $drop.StartFigure()
            $drop.AddBezier(
                [System.Drawing.PointF]::new($s * 0.50, $s * 0.175),
                [System.Drawing.PointF]::new($s * 0.475, $s * 0.27),
                [System.Drawing.PointF]::new($s * 0.295, $s * 0.405),
                [System.Drawing.PointF]::new($s * 0.295, $s * 0.565)
            )
            $drop.AddBezier(
                [System.Drawing.PointF]::new($s * 0.295, $s * 0.565),
                [System.Drawing.PointF]::new($s * 0.295, $s * 0.725),
                [System.Drawing.PointF]::new($s * 0.385, $s * 0.825),
                [System.Drawing.PointF]::new($s * 0.50, $s * 0.825)
            )
            $drop.AddBezier(
                [System.Drawing.PointF]::new($s * 0.50, $s * 0.825),
                [System.Drawing.PointF]::new($s * 0.615, $s * 0.825),
                [System.Drawing.PointF]::new($s * 0.705, $s * 0.725),
                [System.Drawing.PointF]::new($s * 0.705, $s * 0.565)
            )
            $drop.AddBezier(
                [System.Drawing.PointF]::new($s * 0.705, $s * 0.565),
                [System.Drawing.PointF]::new($s * 0.705, $s * 0.405),
                [System.Drawing.PointF]::new($s * 0.525, $s * 0.27),
                [System.Drawing.PointF]::new($s * 0.50, $s * 0.175)
            )
            $drop.CloseFigure()

            $ink = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 37, 39, 43))
            try { $graphics.FillPath($ink, $drop) }
            finally { $ink.Dispose() }
        }
        finally {
            $drop.Dispose()
        }

        $dotRadius = $s * 0.073
        $dotCenterX = $s * 0.555
        $dotCenterY = $s * 0.585
        $cinnabar = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 190, 55, 43))
        try {
            $graphics.FillEllipse(
                $cinnabar,
                $dotCenterX - $dotRadius,
                $dotCenterY - $dotRadius,
                2 * $dotRadius,
                2 * $dotRadius
            )
        }
        finally { $cinnabar.Dispose() }

        $output = [System.Drawing.Bitmap]::new(
            $Size,
            $Size,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
        )
        $outputGraphics = [System.Drawing.Graphics]::FromImage($output)
        $stream = [System.IO.MemoryStream]::new()
        try {
            $outputGraphics.Clear([System.Drawing.Color]::Transparent)
            $outputGraphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
            $outputGraphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $outputGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $outputGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $outputGraphics.DrawImage($bitmap, 0, 0, $Size, $Size)
            $output.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
            return ,$stream.ToArray()
        }
        finally {
            $stream.Dispose()
            $outputGraphics.Dispose()
            $output.Dispose()
        }
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Write-Bytes {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][byte[]]$Bytes
    )
    [System.IO.File]::WriteAllBytes($Path, $Bytes)
}

New-Item -ItemType Directory -Force $resourcesDir, $assetsDir | Out-Null

$sourcePng = New-InkdotPngBytes -Size 512
Write-Bytes -Path (Join-Path $resourcesDir 'inkdot.png') -Bytes $sourcePng

$iconSizes = @(16, 24, 32, 48, 64, 128, 256)
$iconFrames = foreach ($size in $iconSizes) {
    [pscustomobject]@{ Size = $size; Bytes = (New-InkdotPngBytes -Size $size) }
}

$iconPath = Join-Path $resourcesDir 'inkdot.ico'
$iconStream = [System.IO.File]::Create($iconPath)
$writer = [System.IO.BinaryWriter]::new($iconStream)
try {
    $writer.Write([uint16]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]$iconFrames.Count)

    $offset = 6 + (16 * $iconFrames.Count)
    foreach ($frame in $iconFrames) {
        $dimension = if ($frame.Size -eq 256) { [byte]0 } else { [byte]$frame.Size }
        $writer.Write($dimension)
        $writer.Write($dimension)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$frame.Bytes.Length)
        $writer.Write([uint32]$offset)
        $offset += $frame.Bytes.Length
    }
    foreach ($frame in $iconFrames) { $writer.Write($frame.Bytes) }
}
finally {
    $writer.Dispose()
    $iconStream.Dispose()
}

$assetSizes = [ordered]@{
    'Square150x150Logo.scale-100.png' = 150
    'Square150x150Logo.scale-200.png' = 300
    'Square44x44Logo.scale-100.png' = 44
    'Square44x44Logo.scale-200.png' = 88
    'Square44x44Logo.targetsize-256_altform-unplated.png' = 256
    'Square44x44Logo.targetsize-44_altform-unplated.png' = 44
    'StoreLogo.scale-100.png' = 50
    'StoreLogo.scale-200.png' = 100
}

foreach ($asset in $assetSizes.GetEnumerator()) {
    Write-Bytes -Path (Join-Path $assetsDir $asset.Key) -Bytes (New-InkdotPngBytes -Size $asset.Value)
}

Write-Host "Generated Inkdot brand assets in $resourcesDir and $assetsDir"
