@echo off
rem K-GLITCH : ouvre l'installateur dans Chrome ou Edge (le Web MIDI ne marche pas dans Firefox)
set "PAGE=%~dp0K-GLITCH-0.9-installer.html"
for %%B in ("%ProgramFiles%\Google\Chrome\Application\chrome.exe" "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" "%LocalAppData%\Google\Chrome\Application\chrome.exe" "%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe" "%ProgramFiles%\Microsoft\Edge\Application\msedge.exe") do (
  if exist %%B (
    start "" %%B "%PAGE%"
    exit /b 0
  )
)
echo Chrome ou Edge introuvable : ouvrez K-GLITCH-0.9-installer.html avec Chrome ou Edge.
pause
