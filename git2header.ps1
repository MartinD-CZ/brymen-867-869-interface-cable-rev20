$gitVersion =  &"C:\Program Files\Git\cmd\git.exe" describe --long --dirty --tags
#echo $gitVersion

$gitSplit = $gitVersion.Split('-')
$currentDate = Get-Date -Format "dd.MM.yyyy HH:mm"
$targetFile = $PSScriptRoot + "\Inc\version.h"

Clear-Content -Path $targetFile
Add-Content -Path $targetFile -Value ("#ifndef VERSION_H_`n#define VERSION_H_`n")

Add-Content -Path $targetFile -Value ("#define _V_GIT_FULL       (const char*)""" + $gitVersion + """") 

Add-Content -Path $targetFile -Value ("#define _V_BUILD_TAG      (const char*)""" + $gitSplit[0] + """")
Add-Content -Path $targetFile -Value ("#define _V_COMMIT         (const char*)""" + $gitSplit[2] + """") 
Add-Content -Path $targetFile -Value ("#define _V_COMMITS_AHEAD   " + $gitSplit[1]) 

if ($gitSplit[3] -eq "dirty")
{
    Add-Content -Path $targetFile -Value ("#define _V_GIT_DIRTY_FLAG")  
}

Add-Content -Path $targetFile -Value ("#define _V_DATE           (const char*)""" + $currentDate + """") 
Add-Content -Path $targetFile -Value ("#define _V_COMPNAME       (const char*)""" + $env:computername + """") 

Add-Content -Path $targetFile -Value "`n#endif"

echo "git2header executed"