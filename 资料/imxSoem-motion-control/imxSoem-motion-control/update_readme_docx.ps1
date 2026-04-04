$ErrorActionPreference = "Stop"

$outFile = Join-Path (Get-Location) "readme.docx"

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

function PXml {
    param(
        [string]$InnerXml,
        [bool]$Bold = $false,
        [int]$Size = 22,
        [string]$Align = "",
        [string]$Font = "SimSun"
    )

    $pPr = if ($Align) { "<w:pPr><w:jc w:val=`"$Align`"/></w:pPr>" } else { "" }
    $boldXml = if ($Bold) { "<w:b/>" } else { "" }
    "<w:p>$pPr<w:r><w:rPr><w:rFonts w:ascii=`"Calibri`" w:hAnsi=`"Calibri`" w:eastAsia=`"$Font`" w:cs=`"Calibri`"/><w:lang w:val=`"zh-CN`" w:eastAsia=`"zh-CN`"/>$boldXml<w:sz w:val=`"$Size`"/><w:szCs w:val=`"$Size`"/></w:rPr><w:t xml:space=`"preserve`">$InnerXml</w:t></w:r></w:p>"
}

$body = @(
    (PXml -InnerXml "mix_protocol &#35828;&#26126;&#25991;&#26723;" -Bold $true -Size 32 -Align "center" -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#26356;&#26032;&#26085;&#26399;&#65306;2026&#24180;4&#26376;1&#26085;" -Align "center" -Font "Microsoft YaHei"),

    (PXml -InnerXml "&#19968;&#12289;&#39033;&#30446;&#29616;&#29366;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. &#24403;&#21069;&#24037;&#31243;&#24050;&#32463;&#25171;&#36890;&#8220;WSL &#37325;&#32534;&#35793; M7 &#22266;&#20214; + SSH/SCP &#37096;&#32626; + reboot + Linux &#33258;&#21551; main.py + COM3 &#20018;&#21475;&#37319;&#38598;&#8221;&#30340;&#33258;&#21160;&#21270;&#38142;&#36335;&#12290;"),
    (PXml -InnerXml "2. rot &#36724;&#22312;&#24403;&#21069;&#26041;&#26696;&#19979;&#20351;&#29992;&#36895;&#24230;&#27169;&#24335; 3&#65292;&#30001; A &#26680;&#20391; main.py &#36890;&#36807; RPMsg &#19979;&#21457;&#30446;&#26631;&#36895;&#24230;&#65292;M7 &#20391;&#36716;&#25442;&#20026; CANopen SDO/RPDO &#25511;&#21046;&#12290;"),
    (PXml -InnerXml "3. COM3 &#20018;&#21475;&#36755;&#20986;&#24050;&#32463;&#25353;&#35843;&#35797;&#35201;&#27714;&#31934;&#31616;&#65292;&#40664;&#35748;&#21482;&#20445;&#30041;&#31561;&#24453;&#25552;&#31034;&#12289;&#36827;&#20837;&#36816;&#34892;&#25552;&#31034;&#21644;&#36816;&#34892;&#26399;&#29366;&#24577;&#23383;/&#23454;&#38469;&#20301;&#32622;&#21453;&#39304;&#12290;"),

    (PXml -InnerXml "&#20108;&#12289;&#20027;&#35201;&#20837;&#21475;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#26085;&#24120;&#20351;&#29992;&#20837;&#21475;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\&#19968;&#38190;&#32534;&#35793;&#37096;&#32626;&#36816;&#34892;&#26426;&#22120;&#20154;&#24182;&#30417;&#21548;COM3.bat"),
    (PXml -InnerXml "&#20027;&#25511;&#21046;&#33050;&#26412;&#65306;"),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\build_deploy_run_robot_monitor_ddr_release.ps1"),
    (PXml -InnerXml "Linux &#33258;&#21551;&#26381;&#21153;&#65306;"),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\robot-runner.service"),
    (PXml -InnerXml "Linux &#21551;&#21160;&#33050;&#26412;&#65306;"),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\run_robot_remote.sh"),
    (PXml -InnerXml "A &#26680;&#25511;&#21046;&#33050;&#26412;&#65306;"),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\&#26426;&#22120;&#20154;\&#26426;&#22120;&#20154;\motion_control_lib\main.py"),

    (PXml -InnerXml "&#19977;&#12289;&#24403;&#21069;&#36816;&#34892;&#26102;&#24207;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. M7 &#24320;&#26426;&#21518;&#21551;&#21160; RPMsg &#20219;&#21153;&#21644; CAN &#20219;&#21153;&#65292;&#24182;&#23436;&#25104; CAN &#21021;&#22987;&#21270;&#12289;RPDO1 &#26144;&#23556;&#21644; NMT Start&#12290;"),
    (PXml -InnerXml "2. &#23436;&#25104;&#21021;&#22987;&#21270;&#21518;&#65292;M7 &#20391;&#20018;&#21475;&#21482;&#25171;&#21360;&#19968;&#26465;&#31561;&#24453;&#25552;&#31034;&#65306;SMG waiting: CAN initialized, NMT started, waiting for main.py motion command&#12290;"),
    (PXml -InnerXml "3. Linux/A &#26680;&#21551;&#21160; robot-runner.service &#21518;&#20250;&#33258;&#21160;&#25289;&#36215; main.py&#12290;main.py &#36890;&#36807; RPMsg &#19979;&#21457;&#27169;&#24335;&#12289;&#20351;&#33021;&#21644;&#30446;&#26631;&#36895;&#24230;&#21629;&#20196;&#12290;"),
    (PXml -InnerXml "4. M7 &#22312;&#25910;&#21040; main.py &#30340;&#36895;&#24230;&#21629;&#20196;&#21518;&#25171;&#21360; SMG running &#25552;&#31034;&#65292;&#24182;&#24320;&#22987;&#36827;&#20837;&#36816;&#34892;&#26399; CAN &#24490;&#29615;&#12290;"),
    (PXml -InnerXml "5. &#36816;&#34892;&#26399;&#20018;&#21475;&#40664;&#35748;&#21482;&#25171;&#21360; Runtime feedback: status=0x.... actualPosition=.... enabled=1&#65292;&#19981;&#20877;&#21047; Rx MB ID&#12289;write base/read base&#12289;ROT write &#31561;&#36807;&#37327;&#35843;&#35797;&#20449;&#24687;&#12290;"),

    (PXml -InnerXml "&#22235;&#12289;&#26368;&#26032;&#39564;&#35777;&#32467;&#26524;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. 2026&#24180;4&#26376;1&#26085;&#65292;&#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#38142;&#36335;&#24050;&#37325;&#26032;&#36305;&#36890;&#65292;&#20027;&#20837;&#21475; bat &#25191;&#34892;&#36820;&#22238;&#30721;&#20026; 0&#12290;"),
    (PXml -InnerXml "2. &#26368;&#26032;&#26377;&#25928;&#20018;&#21475;&#26085;&#24535;&#65306;D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\serial_logs\robot_run_20260401_112229.log"),
    (PXml -InnerXml "3. &#35813;&#26085;&#24535;&#24050;&#30830;&#35748;&#20986;&#29616;&#31561;&#24453;&#25552;&#31034; SMG waiting&#12289;&#36827;&#20837;&#36816;&#34892;&#25552;&#31034; SMG running&#20197;&#21450;&#36830;&#32493;&#30340; Runtime feedback &#21453;&#39304;&#12290;"),
    (PXml -InnerXml "4. &#35813;&#26085;&#24535;&#20013;&#24050;&#19981;&#20877;&#20986;&#29616; Rx MB ID: 0x581 ...&#12289;write base ...&#12289;read base ...&#12289;ROT write ... &#31561;&#21047;&#23631;&#20449;&#24687;&#12290;"),
    (PXml -InnerXml "5. &#30446;&#21069;&#22312;&#20195;&#30721;&#23618;&#24050;&#32463;&#39069;&#22806;&#25226;&#21021;&#22987;&#21270;&#38454;&#27573;&#30340; RPDO1 armed: ... speed=0 &#26085;&#24535;&#38745;&#38899;&#65292;&#24182;&#24050;&#37325;&#32534;&#35793;&#25104;&#21151;&#12290;&#22914;&#38656;&#35266;&#23519;&#26368;&#26032;&#21551;&#21160;&#26085;&#24535;&#65292;&#21487;&#20877;&#25191;&#34892;&#19968;&#27425;&#20027;&#20837;&#21475; bat &#37319;&#38598;&#26032;&#26085;&#24535;&#12290;"),

    (PXml -InnerXml "&#20116;&#12289;&#24050;&#20462;&#22797;&#30340;&#20851;&#38190;&#38382;&#39064;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. &#37325;&#22797;&#21169;&#30913;&#38382;&#39064;&#24050;&#20462;&#22797;&#12290;&#21407;&#22240;&#26159; reboot &#21518; systemd &#33258;&#21551;&#21644;&#33258;&#21160;&#21270;&#33050;&#26412;&#25163;&#24037;&#37325;&#21551;&#21516;&#26102;&#29983;&#25928;&#65292;&#23548;&#33268;&#39537;&#21160;&#22120;&#37325;&#22797;&#36208; 6 -&gt; 7 -&gt; F &#20351;&#33021;&#27969;&#31243;&#12290;&#29616;&#22312;&#20027;&#33050;&#26412;&#24050;&#26681;&#25454; service/process &#29366;&#24577;&#36991;&#20813;&#37325;&#22797;&#25289;&#36215;&#12290;"),
    (PXml -InnerXml "2. SSH &#24674;&#22797;&#24930;&#23548;&#33268;&#33050;&#26412;&#35823;&#25253;&#22833;&#36133;&#30340;&#38382;&#39064;&#24050;&#37096;&#20998;&#25910;&#25947;&#12290;&#29616;&#22312;&#33050;&#26412;&#24050;&#25226; SMG running &#21644; Runtime feedback &#32435;&#20837;&#25104;&#21151;&#21028;&#23450;&#65292;&#19981;&#20877;&#21482;&#20381;&#36182; SSH &#25104;&#21151;&#36820;&#22238;&#12290;"),
    (PXml -InnerXml "3. COM3 &#26085;&#24535;&#21047;&#23631;&#38382;&#39064;&#24050;&#25353;&#24403;&#21069;&#35843;&#35797;&#38656;&#27714;&#25972;&#29702;&#25104;&#26368;&#23567;&#21270;&#36755;&#20986;&#12290;"),

    (PXml -InnerXml "&#20845;&#12289;&#24403;&#21069;&#20351;&#29992;&#24314;&#35758;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. &#26085;&#24120;&#32852;&#35843;&#26102;&#65292;&#30452;&#25509;&#20351;&#29992;&#19968;&#38190;&#32534;&#35793;&#37096;&#32626;&#36816;&#34892;&#26426;&#22120;&#20154;&#24182;&#30417;&#21548;COM3.bat &#20316;&#20026;&#21807;&#19968;&#20027;&#20837;&#21475;&#12290;"),
    (PXml -InnerXml "2. &#22914;&#26524;&#38656;&#35201;&#21028;&#26029;&#31995;&#32479;&#26159;&#21542;&#36827;&#20837;&#26399;&#26395;&#30340;&#36816;&#34892;&#29366;&#24577;&#65292;&#20248;&#20808;&#30475;&#20018;&#21475;&#20013;&#26159;&#21542;&#20381;&#27425;&#20986;&#29616; SMG waiting&#12289;SMG running&#21644; Runtime feedback&#12290;"),
    (PXml -InnerXml "3. &#22914;&#26524;&#21518;&#32493;&#24076;&#26395;&#20445;&#25345;&#24320;&#26426;&#38745;&#27490;&#65292;&#21017;&#38656;&#35201;&#36827;&#19968;&#27493;&#35843;&#25972; robot-runner.service &#25110; main.py &#30340;&#33258;&#21551;&#31574;&#30053;&#65292;&#32780;&#19981;&#26159;&#20165;&#20462;&#25913; M7 &#20391; CAN &#36816;&#34892;&#24490;&#29615;&#12290;")
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
  <dc:title>mix_protocol 说明文档</dc:title>
  <dc:creator>Codex</dc:creator>
  <cp:lastModifiedBy>Codex</cp:lastModifiedBy>
  <dcterms:created xsi:type="dcterms:W3CDTF">2026-04-01T03:40:00Z</dcterms:created>
  <dcterms:modified xsi:type="dcterms:W3CDTF">2026-04-01T03:40:00Z</dcterms:modified>
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
        <w:lang w:val="zh-CN" w:eastAsia="zh-CN"/>
        <w:sz w:val="22"/>
        <w:szCs w:val="22"/>
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

    $checks = @(
        "&#39033;&#30446;&#29616;&#29366;",
        "&#20027;&#35201;&#20837;&#21475;",
        "SMG waiting",
        "SMG running",
        "Runtime feedback"
    )

    foreach ($check in $checks) {
        if ($xmlText -notmatch [regex]::Escape($check)) {
            throw "Self-check failed."
        }
    }
}
finally {
    $checkZip.Dispose()
}

Get-Item $outFile | Select-Object FullName, Length, LastWriteTime | Format-List
