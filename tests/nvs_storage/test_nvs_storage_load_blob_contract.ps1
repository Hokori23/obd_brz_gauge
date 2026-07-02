$sourcePath = Join-Path $PSScriptRoot "..\..\main\bsp_obd_dsp\nvs_storage.c"
$source = Get-Content $sourcePath -Raw
$loadBlobBody = [regex]::Match(
    $source,
    '(?s)static esp_err_t load_blob\(.*?\n\}(?=\s*static esp_err_t load_blob_or_create\()'
).Value

if ([string]::IsNullOrWhiteSpace($loadBlobBody)) {
    throw "Failed to locate load_blob() body in nvs_storage.c"
}

if ($loadBlobBody -match 'memset\(out,\s*0,\s*len\);') {
    throw "load_blob() must not zero the destination buffer on read failure"
}

if ($loadBlobBody -match 'save_blob\(ns,\s*key,\s*out,\s*len\)') {
    throw "load_blob() must not overwrite persisted data as part of its failure path"
}

if ($source -notmatch 'static esp_err_t load_blob_or_create\(') {
    throw "nvs_storage.c must keep a separate create-on-missing helper instead of mixing that policy into load_blob()"
}

Write-Output "nvs storage load_blob contract checks passed"
