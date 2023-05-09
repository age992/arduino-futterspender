# Set the path to the folder containing the C++ files
$path = ".\tgrbxsvr"

# Set the name of the output file
$outputFile = ".\output.cpp"

# Create or overwrite the output file
New-Item -ItemType File -Path $outputFile -Force | Out-Null

# Get all C++ header and source files in the folder
$files = Get-ChildItem -Path $sourcePath -Include "*.ino","*.h", "*.cpp" -Recurse | Where-Object { $_.Attributes -ne "Directory" }

# Loop through each file
foreach ($file in $files) {
    # Get the content of the file
    $content = Get-Content $file.FullName

    # Write the start mark to the output file
    Add-Content -Path $outputFile -Value "/* === Start of $($file.Name) === */"

    # Append the file content to the output file
    Add-Content -Path $outputFile -Value $content

    # Write the end mark to the output file
    Add-Content -Path $outputFile -Value "/* === End of $($file.Name) === */`n"
}
