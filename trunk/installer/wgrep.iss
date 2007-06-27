; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=wgrep
AppVerName=wgrep 0.5.0
DefaultDirName={pf}\wgrep
DefaultGroupName=wgrep
UsePreviousAppDir=yes
OutputBaseFilename=wgrep

[Files]
Source: "wgrep.exe"; DestDir: "{app}"
;Source: "register_wgrep.reg"; DestDir: "{app}"
;Source: "unregister_wgrep.reg"; DestDir: "{app}"

;[Run]
;Filename: "{app}\register_wgrep.reg"; Description: "Integrate wgrep in your shell"; StatusMsg: "Integrating wgrep..."; Flags: postinstall shellexec

;[UninstallRun]
;Filename: "{app}\unregister_wgrep.reg"; StatusMsg: "Uninstalling wgrep..."; Flags: shellexec waituntilterminated

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Classes\Folder\shell\wgrep"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Folder\shell\wgrep\Command"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Folder\shell\wgrep\Command"; ValueType: string; ValueName: ""; ValueData: "{app}\wgrep.exe %1"; Flags: uninsdeletekey

Root: HKLM; Subkey: "SOFTWARE\Classes\*\shell\wgrep"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\*\shell\wgrep\Command"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\*\shell\wgrep\Command"; ValueType: string; ValueName: ""; ValueData: "{app}\wgrep.exe %1"; Flags: uninsdeletekey

Root: HKCU; Subkey: "SOFTWARE\wgrep"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\wgrep"; ValueType: string; ValueName: "wgrepPath"; ValueData: "{app}"; Flags: uninsdeletekey

Root: HKCU; Subkey: "SOFTWARE\wgrep\regexps"; Flags: uninsdeletekey

Root: HKCU; Subkey: "SOFTWARE\wgrep\filters"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\wgrep\filters"; ValueType: string; ValueName: "0"; ValueData: "*.c;*.cpp;*.cxx;*.tli;*.h;*.tlh;*.inl;*.rc;*.tcl"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\wgrep\filters"; ValueType: string; ValueName: "1"; ValueData: "*.txt"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\wgrep\filters"; ValueType: string; ValueName: "2"; ValueData: "*.*"; Flags: uninsdeletekey

Root: HKCU; Subkey: "SOFTWARE\wgrep\folders"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\wgrep\folders"; ValueType: string; ValueName: "0"; ValueData: "c:\"; Flags: uninsdeletekey

[Code]
procedure InitializeWizard();
var
  PSPadPath: String;
begin
  if RegKeyExists(HKEY_CURRENT_USER, 'Software\PSPad') then
  begin
    if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\PSPad', 'PSPadPath', PSPadPath) then
    begin
      StringChangeEx(PSPadPath, '"', '', True);
      RegWriteStringValue(HKEY_CURRENT_USER, 'SOFTWARE\wgrep', 'EditorName', 'PSPad');
      RegWriteStringValue(HKEY_CURRENT_USER, 'SOFTWARE\wgrep', 'EditorPath', PSPadPath);
    end else begin
      RegWriteStringValue(HKEY_CURRENT_USER, 'SOFTWARE\wgrep', 'EditorName', 'Notepad');
      RegWriteStringValue(HKEY_CURRENT_USER, 'SOFTWARE\wgrep', 'EditorPath', 'Notepad.exe');
    end
  end;
end;

