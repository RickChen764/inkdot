param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$SourceImage = (Join-Path (Split-Path -Parent $PSScriptRoot) 'resources/inkdot-source.png')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$resourcesDir = Join-Path $RepositoryRoot 'resources'
$assetsDir = Join-Path $RepositoryRoot 'packaging/msix/Assets'

if (-not (Test-Path -LiteralPath $SourceImage)) {
    throw "Approved source artwork not found: $SourceImage"
}

function New-InkdotPngBytes {
    param([Parameter(Mandatory)][int]$Size)

    $source = [System.Drawing.Bitmap]::new($SourceImage)
    $output = [System.Drawing.Bitmap]::new(
        $Size,
        $Size,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )
    $graphics = [System.Drawing.Graphics]::FromImage($output)
    $stream = [System.IO.MemoryStream]::new()
    try {
        $graphics.Clear([System.Drawing.Color]::FromArgb(255, 247, 243, 231))
        $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
        $graphics.InterpolationMode = if ($Size -le 32) {
            [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBilinear
        } else {
            [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        }
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality

        # The approved source is 1254 x 1254. This crop is centered on the
        # actual calligraphic stroke, retaining about 15% breathing room and
        # all of the dry-brush entry at the upper left.
        $sourceRect = [System.Drawing.RectangleF]::new(190, 205, 925, 925)
        $destinationRect = [System.Drawing.RectangleF]::new(0, 0, $Size, $Size)
        $graphics.DrawImage(
            $source,
            $destinationRect,
            $sourceRect,
            [System.Drawing.GraphicsUnit]::Pixel
        )

        $output.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
        return ,$stream.ToArray()
    }
    finally {
        $stream.Dispose()
        $graphics.Dispose()
        $output.Dispose()
        $source.Dispose()
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

Write-Bytes -Path (Join-Path $resourcesDir 'inkdot.png') -Bytes (New-InkdotPngBytes -Size 512)

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

Write-Host "Generated Inkdot assets from approved artwork: $SourceImage"
