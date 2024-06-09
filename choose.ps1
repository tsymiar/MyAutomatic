# set-executionpolicy remotesigned
param (
#       [Parameter(Mandatory=$true)]
        [string]$option
    )

$libSymbols = "BOOST", "CEFDIR", "QTDIR", "OPENCV", "OPENSSL", "PTHD_LIB86", "JAVA_HOME", "ZLIB", "libPNG"

if ($option -eq "clean")
{
	foreach ($lib in $libSymbols) {
		[Environment]::SetEnvironmentVariable($lib, $null, "User")
		Write-Host "$option '$lib' OK"
	}
}
elseif ($option)
{
	Write-Output "Not support '$option' option"
}
else
{
	Add-Type -AssemblyName System.Windows.Forms

	$browser = New-Object System.Windows.Forms.FolderBrowserDialog
	$browser.Description = 'Select a folder to set Environment'
	$browser.RootFolder = [Environment+SpecialFolder]::Desktop
	$browser.ShowNewFolderButton = $false

	$form = New-Object Windows.Forms.Form
	$onFormClosingScript = {
		param($sender, $e)
		Write-Host "closing window..."
	}
	$form.add_FormClosing($onFormClosingScript)

	foreach ($lib in $libSymbols) {
		echo "select '$lib' folder Path"
		$result = $browser.ShowDialog()
		if ($result -eq [Windows.Forms.DialogResult]::OK) {
			$folderPath = $browser.SelectedPath
			setx $lib $folderPath
			echo "set '$lib = $folderPath' OK"
		} else {
			echo "NOT set '$lib' library Path"
			break
		}
	}
}
