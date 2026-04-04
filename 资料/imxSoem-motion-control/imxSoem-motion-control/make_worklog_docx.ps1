$ErrorActionPreference = "Stop"

$outFile = Join-Path (Get-Location) "worklog_2026-03-31.docx"

function U([int[]]$codes) {
    -join ($codes | ForEach-Object { [char]$_ })
}

function Add-Entry {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$Name,
        [string]$Text
    )

    $entry = $Archive.CreateEntry($Name)
    $stream = $entry.Open()
    try {
        $bytes = [System.Text.UTF8Encoding]::new($false).GetBytes($Text)
        $stream.Write($bytes, 0, $bytes.Length)
    }
    finally {
        $stream.Dispose()
    }
}

function P {
    param(
        [string]$InnerXml,
        [bool]$Bold = $false,
        [int]$Size = 22,
        [string]$Align = "",
        [string]$Font = "SimSun"
    )

    $pPr = if ($Align) { "<w:pPr><w:jc w:val=`"$Align`"/></w:pPr>" } else { "" }
    $boldXml = if ($Bold) { "<w:b/>" } else { "" }
    "<w:p>$pPr<w:r><w:rPr><w:rFonts w:ascii=`"Calibri`" w:hAnsi=`"Calibri`" w:eastAsia=`"$Font`"/>$boldXml<w:sz w:val=`"$Size`"/></w:rPr><w:t xml:space=`"preserve`">$InnerXml</w:t></w:r></w:p>"
}

$titleText = U @(20170,26085,24037,20316,26085,24535)
$dateText = U @(26085,26399,32,50,48,50,54,24180,51,26376,51,49,26085)
$s1 = U @(19968,12289,20170,26085,23436,25104,20869,23481)
$s2 = U @(20108,12289,20851,38190,38382,39064,19982,35843,22788,29702)
$s3 = U @(19977,12289,39564,35777,32467,26524)
$s4 = U @(22235,12289,36755,20986,29289)
$s5 = U @(20116,12289,26126,26085,35745,21010)

$body = @(
    (P -InnerXml "&#20170;&#26085;&#24037;&#20316;&#26085;&#24535;" -Bold $true -Size 32 -Align "center" -Font "Microsoft YaHei"),
    (P -InnerXml "&#26085;&#26399;&#65306;2026&#24180;3&#26376;31&#26085;" -Align "center"),
    (P -InnerXml "&#19968;&#12289;&#20170;&#26085;&#23436;&#25104;&#20869;&#23481;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (P -InnerXml "1. &#22260;&#32469; i.MX8MP M7 &#22266;&#20214;&#24037;&#31243;&#65292;&#25345;&#32493;&#23436;&#21892; WSL &#32534;&#35793;&#12289;SSH &#37096;&#32626;&#12289;&#19979;&#21457; .bin&#12289;reboot&#12289;&#21551;&#21160;&#26426;&#22120;&#20154;&#31243;&#24207;&#20197;&#21450; COM3 &#20018;&#21475;&#30417;&#21548;&#30340;&#19968;&#38190;&#21270;&#38142;&#36335;&#12290;"),
    (P -InnerXml "2. &#23436;&#25104; /home/main.py&#12289;/home/librobot.so.1.0.0&#12289;/home/run_robot_remote.sh &#20197;&#21450; robot-runner.service &#30340;&#37096;&#32626;&#25910;&#25947;&#65292;&#30830;&#20445;&#24320;&#21457;&#26495;&#36816;&#34892;&#25991;&#20214;&#36335;&#24452;&#22266;&#23450;&#12289;&#21487;&#37325;&#22797;&#19979;&#21457;&#12290;"),
    (P -InnerXml "3. &#25345;&#32493;&#32852;&#35843; main.py -&gt; RPMsg -&gt; Weld_Direct_SetTargetVel -&gt; CAN &#20282;&#26381;&#38142;&#36335;&#65292;&#30830;&#35748; rot &#30005;&#26426;&#21487;&#20197;&#22312;&#36895;&#24230;&#27169;&#24335; 3 &#19979;&#25345;&#32493;&#36716;&#21160;&#12290;"),
    (P -InnerXml "4. &#23436;&#25104;&#33258;&#21160;&#21270;&#33050;&#26412;&#30340;&#25104;&#21151;&#21028;&#23450;&#36923;&#36753;&#20462;&#27491;&#65292;&#35299;&#20915;&#20102; reboot &#21518; SSH &#24674;&#22797;&#36739;&#24930;&#26102;&#33050;&#26412;&#35823;&#25253;&#22833;&#36133;&#30340;&#38382;&#39064;&#12290;"),
    (P -InnerXml "5. &#20248;&#21270; run_robot_remote.sh &#19982; robot-runner.service&#65292;&#22686;&#21152;&#26085;&#24535;&#33853;&#30424;&#12289;&#22833;&#36133;&#37325;&#35797;&#12289;&#24037;&#20316;&#30446;&#24405;&#22266;&#23450;&#12289;&#20018;&#21475;&#20889;&#20837;&#23481;&#38169;&#31561;&#26426;&#21046;&#65292;&#25552;&#39640;&#20102;&#21551;&#21160;&#31283;&#23450;&#24615;&#12290;"),
    (P -InnerXml "&#20108;&#12289;&#20851;&#38190;&#38382;&#39064;&#19982;&#35843;&#35797;&#22788;&#29702;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (P -InnerXml "1. &#23450;&#20301;&#21040;&#27492;&#21069;&#33258;&#21160;&#21270;&#19981;&#31283;&#23450;&#30340;&#20027;&#35201;&#21407;&#22240;&#21253;&#25324;&#65306;reboot &#21518; SSH &#24674;&#22797;&#24930;&#12289;systemd &#21551;&#21160;&#29615;&#22659;&#19982;&#25163;&#24037;&#25191;&#34892;&#29615;&#22659;&#23384;&#22312;&#24046;&#24322;&#12289;&#20018;&#21475;&#36755;&#20986;&#34987;&#36807;&#26089;&#24403;&#20316;&#8220;&#25104;&#21151;&#20449;&#21495;&#8221;&#12290;"),
    (P -InnerXml "2. &#36890;&#36807;&#25163;&#24037; SSH &#25191;&#34892; python3 /home/main.py &#19982;&#19968;&#38190;&#33258;&#21160;&#21270;&#25191;&#34892;&#30340;&#23545;&#27604;&#65292;&#30830;&#35748;&#26495;&#31471;&#31243;&#24207;&#23454;&#38469;&#33021;&#22815;&#25104;&#21151;&#19979;&#21457; Weld_Direct_SetTargetVel&#65292;&#24182;&#19988;&#32534;&#30721;&#22120;&#35835;&#25968;&#20250;&#25345;&#32493;&#22686;&#38271;&#12290;"),
    (P -InnerXml "3. &#23558;&#26495;&#31471;&#36816;&#34892;&#26085;&#24535;&#21516;&#27493;&#21040; /home/robot_runtime.log&#65292;&#24182;&#22312;&#19968;&#38190;&#33050;&#26412;&#20013;&#22686;&#21152;&#23545;&#25991;&#20214;&#26085;&#24535;&#21644;&#20018;&#21475;&#26085;&#24535;&#30340;&#21452;&#37325;&#30830;&#35748;&#65292;&#26368;&#32456;&#25226;&#8220;&#21151;&#33021;&#24322;&#24120;&#8221;&#25910;&#25947;&#20026;&#8220;&#25104;&#21151;&#21028;&#23450;&#26465;&#20214;&#19981;&#22815;&#23485;&#8221;&#30340;&#33050;&#26412;&#23618;&#38382;&#39064;&#12290;"),
    (P -InnerXml "4. &#37325;&#26032;&#36816;&#34892;&#23436;&#25972;&#37325;&#32534;&#35793;&#38142;&#36335;&#21518;&#65292;COM3 &#20018;&#21475;&#26085;&#24535;&#26126;&#30830;&#26174;&#31034; modeDisplay=3&#12289;target_vel=3000000&#12289;actualSpeed &#25345;&#32493;&#38750;&#38646;&#12289;actualPosition &#25345;&#32493;&#22686;&#38271;&#65292;&#21487;&#20197;&#30830;&#35748;&#30005;&#26426;&#22788;&#20110;&#25345;&#32493;&#36716;&#21160;&#29366;&#24577;&#12290;"),
    (P -InnerXml "&#19977;&#12289;&#39564;&#35777;&#32467;&#26524;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (P -InnerXml "1. &#26368;&#26032;&#23436;&#25972;&#19968;&#38190;&#38142;&#36335;&#24050;&#25104;&#21151;&#25191;&#34892;&#65292;&#20837;&#21475;&#20026;&#65306;mix_protocol/armgcc/&#19968;&#38190;&#32534;&#35793;&#37096;&#32626;&#36816;&#34892;&#26426;&#22120;&#20154;&#24182;&#30417;&#21548;COM3.bat&#12290;"),
    (P -InnerXml "2. &#26368;&#26032;&#26377;&#25928;&#20018;&#21475;&#26085;&#24535;&#20026;&#65306;robot_run_20260331_183029.log&#12290;"),
    (P -InnerXml "3. &#26085;&#24535;&#20013;&#21487;&#35266;&#23519;&#21040;&#20197;&#19979;&#20851;&#38190;&#29366;&#24577;&#65306;&#36895;&#24230;&#27169;&#24335;&#20026; 3&#12289;&#30446;&#26631;&#36895;&#24230;&#20026; 3000000&#12289;&#23454;&#38469;&#36895;&#24230;&#38271;&#26102;&#38388;&#20445;&#25345;&#22312;&#32422; 2.9M &#21040; 3.2M &#20043;&#38388;&#12289;&#20301;&#32622;&#21453;&#39304;&#25345;&#32493;&#19978;&#21319;&#12290;"),
    (P -InnerXml "4. &#30446;&#21069;&#33258;&#21160;&#21270;&#38142;&#36335;&#24050;&#32463;&#33021;&#22815;&#31283;&#23450;&#23436;&#25104;&#37325;&#32534;&#35793;&#12289;&#37096;&#32626;&#12289;&#37325;&#21551;&#12289;&#36816;&#34892;&#20197;&#21450;&#20018;&#21475;&#37319;&#38598;&#65292;&#20855;&#22791;&#26085;&#24120;&#20351;&#29992;&#26465;&#20214;&#12290;"),
    (P -InnerXml "&#22235;&#12289;&#36755;&#20986;&#29289;&#21644;&#25913;&#21160;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (P -InnerXml "1. &#33258;&#21160;&#21270;&#20027;&#20837;&#21475;&#33050;&#26412;&#24050;&#32463;&#25910;&#25947;&#20026;&#37325;&#32534;&#35793;&#29256;&#26412;&#65292;&#29992;&#20110;&#26085;&#24120;&#25191;&#34892;&#30340;&#20027;&#35201;&#20837;&#21475;&#20026;&#19968;&#38190;&#32534;&#35793;&#37096;&#32626;&#36816;&#34892;&#26426;&#22120;&#20154;&#24182;&#30417;&#21548;COM3.bat&#12290;"),
    (P -InnerXml "2. run_robot_remote.sh&#12289;robot-runner.service&#12289;build_deploy_run_robot_monitor_ddr_release.ps1 &#31561;&#20851;&#38190;&#25991;&#20214;&#22343;&#24050;&#26681;&#25454;&#23454;&#38469;&#35843;&#35797;&#32467;&#26524;&#36827;&#34892;&#20462;&#25913;&#12290;"),
    (P -InnerXml "3. &#24050;&#39069;&#22806;&#29983;&#25104;&#24037;&#20316;&#26085;&#24535; docx &#25991;&#26723;&#65292;&#26041;&#20415;&#30452;&#25509;&#25552;&#20132;&#32473;&#32452;&#38271;&#12290;"),
    (P -InnerXml "&#20116;&#12289;&#26126;&#26085;&#35745;&#21010;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (P -InnerXml "1. &#32487;&#32493;&#20248;&#21270;&#25972;&#26465;&#37325;&#32534;&#35793;&#38142;&#36335;&#30340;&#24635;&#32791;&#26102;&#65292;&#37325;&#28857;&#25910;&#25947; reboot &#21518;&#30340;&#31561;&#24453;&#31574;&#30053;&#21644;&#21518;&#21488; SSH &#37325;&#35797;&#25552;&#31034;&#12290;"),
    (P -InnerXml "2. &#35270;&#23454;&#38469;&#38656;&#27714;&#20915;&#23450;&#26159;&#21542;&#20877;&#34917;&#20805;&#26356;&#23433;&#38745;&#30340;&#21457;&#24067;&#29256;&#20837;&#21475;&#65292;&#36827;&#19968;&#27493;&#20943;&#23569; WSL &#21644;&#32534;&#35793;&#38454;&#27573;&#30340;&#20987;&#20313;&#36755;&#20986;&#12290;"),
    (P -InnerXml "3. &#33509;&#21518;&#32493;&#36827;&#20837;&#19994;&#21153;&#32852;&#35843;&#38454;&#27573;&#65292;&#21487;&#32487;&#32493;&#25193;&#23637; rot &#20197;&#22806;&#36724;&#25110;&#28938;&#25509;&#27969;&#31243;&#21629;&#20196;&#30340;&#33258;&#21160;&#21270;&#39564;&#35777;&#12290;")
)

$contentTypes = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
  <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
</Types>
"@

$rels = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>
"@

$docRels = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>
"@

$core = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>$titleText</dc:title>
  <dc:creator>Codex</dc:creator>
  <cp:lastModifiedBy>Codex</cp:lastModifiedBy>
  <dcterms:created xsi:type="dcterms:W3CDTF">2026-03-31T19:10:00Z</dcterms:created>
  <dcterms:modified xsi:type="dcterms:W3CDTF">2026-03-31T19:10:00Z</dcterms:modified>
</cp:coreProperties>
"@

$app = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">
  <Application>Codex</Application>
</Properties>
"@

$styles = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault>
      <w:rPr>
        <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri" w:eastAsia="SimSun" w:cs="Calibri"/>
        <w:sz w:val="22"/>
        <w:lang w:val="zh-CN" w:eastAsia="zh-CN"/>
      </w:rPr>
    </w:rPrDefault>
    <w:pPrDefault>
      <w:pPr>
        <w:spacing w:after="120" w:line="360" w:lineRule="auto"/>
      </w:pPr>
    </w:pPrDefault>
  </w:docDefaults>
</w:styles>
"@

$document = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:wpc="http://schemas.microsoft.com/office/word/2010/wordprocessingCanvas" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:o="urn:schemas-microsoft-com:office:office" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math" xmlns:v="urn:schemas-microsoft-com:vml" xmlns:wp14="http://schemas.microsoft.com/office/word/2010/wordprocessingDrawing" xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing" xmlns:w10="urn:schemas-microsoft-com:office:word" xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml" xmlns:wpg="http://schemas.microsoft.com/office/word/2010/wordprocessingGroup" xmlns:wpi="http://schemas.microsoft.com/office/word/2010/wordprocessingInk" xmlns:wne="http://schemas.microsoft.com/office/2006/wordml" xmlns:wps="http://schemas.microsoft.com/office/word/2010/wordprocessingShape" mc:Ignorable="w14 wp14">
  <w:body>
    $($body -join "`n    ")
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="708" w:footer="708" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
"@

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

if (Test-Path $outFile) {
    Remove-Item $outFile -Force
}

$zip = [System.IO.Compression.ZipFile]::Open($outFile, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    Add-Entry -Archive $zip -Name "[Content_Types].xml" -Text $contentTypes
    Add-Entry -Archive $zip -Name "_rels/.rels" -Text $rels
    Add-Entry -Archive $zip -Name "docProps/core.xml" -Text $core
    Add-Entry -Archive $zip -Name "docProps/app.xml" -Text $app
    Add-Entry -Archive $zip -Name "word/document.xml" -Text $document
    Add-Entry -Archive $zip -Name "word/styles.xml" -Text $styles
    Add-Entry -Archive $zip -Name "word/_rels/document.xml.rels" -Text $docRels
}
finally {
    $zip.Dispose()
}

$checkZip = [System.IO.Compression.ZipFile]::OpenRead($outFile)
try {
    $entry = $checkZip.GetEntry("word/document.xml")
    $reader = New-Object System.IO.StreamReader($entry.Open(), [System.Text.Encoding]::UTF8)
    try {
        $xmlText = $reader.ReadToEnd()
    }
    finally {
        $reader.Dispose()
    }

    $doc = New-Object System.Xml.XmlDocument
    $doc.LoadXml($xmlText)
    $innerText = $doc.DocumentElement.InnerText
    if (($innerText -notmatch [regex]::Escape($titleText)) -or ($innerText -notmatch [regex]::Escape($s1)) -or ($innerText -notmatch [regex]::Escape($s3))) {
        throw "Self-check failed"
    }
}
finally {
    $checkZip.Dispose()
}

Get-Item $outFile | Select-Object FullName, Length, LastWriteTime | Format-List
