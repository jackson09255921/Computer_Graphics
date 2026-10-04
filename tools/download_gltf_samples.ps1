param(
    [string]$Destination = "LabX/data/external/gltf_samples"
)

$ErrorActionPreference = "Stop"
$repository = "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models"
$models = @(
    "AlphaBlendModeTest",
    "Avocado",
    "BoomBox",
    "BoxTextured",
    "ClearCoatTest",
    "DamagedHelmet",
    "Duck",
    "Lantern",
    "SheenTestGrid",
    "ToyCar",
    "WaterBottle"
)

New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$manifest = @()
foreach ($model in $models) {
    $directory = Join-Path $Destination $model
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $asset = Join-Path $directory "$model.glb"
    $readme = Join-Path $directory "README.md"
    if (!(Test-Path $asset) -or (Get-Item $asset).Length -eq 0) {
        & curl.exe --fail --location --silent --show-error --output $asset "$repository/$model/glTF-Binary/$model.glb"
        if ($LASTEXITCODE -ne 0) { throw "failed to download $model GLB" }
    }
    & curl.exe --fail --location --silent --show-error --output $readme "$repository/$model/README.md"
    if ($LASTEXITCODE -ne 0) { throw "failed to download $model README" }
    $hash = (Get-FileHash -Algorithm SHA256 -Path $asset).Hash.ToLowerInvariant()
    $manifest += [ordered]@{
        name = $model
        file = "$model/$model.glb"
        source = "https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/$model"
        license = "$model/README.md"
        bytes = (Get-Item $asset).Length
        sha256 = $hash
    }
}

$transformModel = "TextureTransformTest"
$transformDirectory = Join-Path $Destination $transformModel
New-Item -ItemType Directory -Force -Path $transformDirectory | Out-Null
$transformFiles = @("TextureTransformTest.gltf", "TextureTransformTest.bin", "Arrow.png",
    "Correct.png", "Error.png", "NotSupported.png", "UV.png")
foreach ($file in $transformFiles) {
    $target = Join-Path $transformDirectory $file
    & curl.exe --fail --location --silent --show-error --output $target "$repository/$transformModel/glTF/$file"
    if ($LASTEXITCODE -ne 0) { throw "failed to download $transformModel/$file" }
}
& curl.exe --fail --location --silent --show-error --output (Join-Path $transformDirectory "README.md") `
    "$repository/$transformModel/README.md"
if ($LASTEXITCODE -ne 0) { throw "failed to download $transformModel README" }
$transformAsset = Join-Path $transformDirectory "TextureTransformTest.gltf"
$dependencies = @()
foreach ($file in $transformFiles | Where-Object { $_ -ne "TextureTransformTest.gltf" }) {
    $dependency = Join-Path $transformDirectory $file
    $dependencies += [ordered]@{
        file = "$transformModel/$file"
        bytes = (Get-Item $dependency).Length
        sha256 = (Get-FileHash -Algorithm SHA256 -Path $dependency).Hash.ToLowerInvariant()
    }
}
$manifest += [ordered]@{
    name = $transformModel
    file = "$transformModel/TextureTransformTest.gltf"
    source = "https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/$transformModel"
    license = "$transformModel/README.md"
    bytes = (Get-Item $transformAsset).Length
    sha256 = (Get-FileHash -Algorithm SHA256 -Path $transformAsset).Hash.ToLowerInvariant()
    dependencies = $dependencies
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $Destination "manifest.json")
Write-Host "Downloaded $($models.Count + 1) licensed Khronos glTF sample assets to $Destination"
