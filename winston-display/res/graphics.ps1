
function Get-SVG-And-PNGrize($name, $size) {
	
	"Doing ${name} for ${size}x${size}"

	Invoke-RestMethod -method Get -URI "https://raw.githubusercontent.com/google/material-design-icons/refs/heads/master/symbols/web/${name}/materialsymbolsoutlined/${name}_48px.svg" -OutFile "${name}.svg" 
	Start-Sleep 1
	inkscape.exe -w $size -h $size "${name}.svg" -o "${name}_${size}.png"
	Start-Sleep 10
	python3.12.exe ..\..\LVGLImage.py --ofmt C --cf RGB565A8 -o "..\\${name}_${size}.c" "${name}_${size}.png"
	
	Write-Output "#include `"icons\${name}_${size}.c\${name}_${size}.c`"" >> ..\..\lvgl_graphics.utf8
}

Remove-Item lvgl_graphics.c
Remove-Item lvgl_graphics.utf8
Write-Output "#define LV_LVGL_H_INCLUDE_SIMPLE" >> lvgl_graphics.utf8

# download icons from google and convert svg to png and c file
pushd
cd icons\graphics
# delete old
Remove-Item *.svg 
Remove-Item *.png
Remove-Item -Recourse ..\*.c

# 24x24
Get-SVG-And-PNGrize arrow_back 24

# 48x48
Get-SVG-And-PNGrize close 48
Get-SVG-And-PNGrize movie 48
Get-SVG-And-PNGrize quiz 48
Get-SVG-And-PNGrize refresh 48
Get-SVG-And-PNGrize settings 48
Get-SVG-And-PNGrize train 48
Get-SVG-And-PNGrize monitor_heart 48

popd
gc -en utf8 lvgl_graphics.utf8 | Out-File -en ascii lvgl_graphics.c