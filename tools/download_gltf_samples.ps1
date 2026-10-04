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
    "DamagedHelmet",
    "Duck",
    "Lantern",
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
$manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $Destination "manifest.json")
Write-Host "Downloaded $($models.Count) licensed Khronos glTF sample assets to $Destination"
