$ErrorActionPreference = "Stop"

$outFile = Join-Path (Get-Location) "mix_protocol_note_2026-04-01.docx"

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
    (PXml -InnerXml "mix_protocol &#33258;&#21160;&#21270;&#38142;&#36335;&#35828;&#26126;" -Bold $true -Size 32 -Align "center" -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#26085;&#26399;&#65306;2026&#24180;4&#26376;1&#26085;" -Align "center" -Size 22 -Font "Microsoft YaHei"),

    (PXml -InnerXml "&#19968;&#12289;&#25991;&#26723;&#30446;&#30340;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#26412;&#25991;&#26723;&#29992;&#20110;&#35828;&#26126; imxSoem-motion-control &#24037;&#31243;&#24403;&#21069;&#24050;&#25171;&#36890;&#30340;&#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#38142;&#36335;&#12289;&#24320;&#26426;&#33258;&#21551;&#36816;&#34892;&#26041;&#24335;&#12289;&#20018;&#21475;&#39564;&#35777;&#32467;&#26524;&#20197;&#21450;&#26412;&#36718;&#25490;&#26597;&#21040;&#30340;&#37325;&#28857;&#38382;&#39064;&#21644;&#20462;&#22797;&#32467;&#26524;&#12290;"),

    (PXml -InnerXml "&#20108;&#12289;&#24403;&#21069;&#33258;&#21160;&#21270;&#38142;&#36335;&#27010;&#36848;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#24403;&#21069;&#26085;&#24120;&#20351;&#29992;&#30340;&#20027;&#20837;&#21475;&#20026;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\&#19968;&#38190;&#32534;&#35793;&#37096;&#32626;&#36816;&#34892;&#26426;&#22120;&#20154;&#24182;&#30417;&#21548;COM3.bat"),
    (PXml -InnerXml "&#36825;&#20010;&#20837;&#21475;&#20250;&#23436;&#25104;&#20197;&#19979;&#21160;&#20316;&#65306;"),
    (PXml -InnerXml "1. &#22312; Windows &#31471;&#35843;&#29992; PowerShell &#20027;&#33050;&#26412;&#65292;&#36827;&#19968;&#27493;&#36890;&#36807; WSL &#20351;&#29992; SDK_2_14_0_EVK-MIMX8MP1.zip &#19982; arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.zip &#37325;&#32534;&#35793; M7 ddr_release &#22266;&#20214;&#12290;"),
    (PXml -InnerXml "2. &#33258;&#21160;&#25226;&#26032;&#29983;&#25104;&#30340; .bin &#25991;&#20214;&#37096;&#32626;&#21040;&#24320;&#21457;&#26495;&#12290;"),
    (PXml -InnerXml "3. &#21516;&#27493;&#37096;&#32626; /home/main.py&#12289;/home/librobot.so.1.0.0&#12289;/home/run_robot_remote.sh &#20197;&#21450; systemd &#26381;&#21153;&#25991;&#20214;&#12290;"),
    (PXml -InnerXml "4. &#25191;&#34892; reboot&#65292;&#24182;&#22312;&#24320;&#21457;&#26495;&#37325;&#21551;&#23436;&#25104;&#21518;&#30001; systemd &#33258;&#21160;&#25289;&#36215;&#26426;&#22120;&#20154;&#36816;&#34892;&#33050;&#26412;&#12290;"),
    (PXml -InnerXml "5. &#25171;&#24320; COM3 &#20018;&#21475;&#65292;&#20197; 115200 &#27874;&#29305;&#29575;&#37319;&#38598;&#22266;&#20214;&#21551;&#21160;&#26085;&#24535;&#21644; CAN &#36816;&#34892;&#21453;&#39304;&#12290;"),
    (PXml -InnerXml "PowerShell &#20027;&#33050;&#26412;&#36335;&#24452;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\build_deploy_run_robot_monitor_ddr_release.ps1"),
    (PXml -InnerXml "systemd &#26381;&#21153;&#25991;&#20214;&#36335;&#24452;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\robot-runner.service"),
    (PXml -InnerXml "&#26495;&#31471;&#21551;&#21160;&#33050;&#26412;&#36335;&#24452;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\run_robot_remote.sh"),

    (PXml -InnerXml "&#19977;&#12289;&#24403;&#21069;&#36816;&#34892;&#36827;&#31243;&#35828;&#26126;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. M7 &#20391;&#22312; main_remote.c &#20013;&#21551;&#21160; FreeRTOS &#20219;&#21153;&#65292;&#20854;&#20013;&#21253;&#25324; RPMsg &#20219;&#21153;&#19982; CAN &#20219;&#21153;&#12290;"),
    (PXml -InnerXml "2. Linux/A &#26680;&#20391;&#22312;&#24320;&#26426;&#21518;&#20250;&#30001; robot-runner.service &#33258;&#21160;&#25191;&#34892; run_robot_remote.sh&#65292;&#35813;&#33050;&#26412;&#20250;&#36827;&#19968;&#27493;&#21551;&#21160; /home/main.py&#12290;"),
    (PXml -InnerXml "3. main.py &#36890;&#36807; RPMsg &#21521; M7 &#19979;&#21457; rot &#36724;&#30340;&#27169;&#24335;&#12289;&#20351;&#33021;&#21644;&#30446;&#26631;&#36895;&#24230;&#21629;&#20196;&#12290;"),
    (PXml -InnerXml "4. servo_can.c &#36127;&#36131;&#25226; rot &#30340;&#25511;&#21046;&#21442;&#25968;&#36716;&#25442;&#20026; CANopen SDO/RPDO &#25805;&#20316;&#65292;&#24182;&#25345;&#32493;&#35835;&#21462;&#27169;&#24335;&#12289;&#29366;&#24577;&#12289;&#36895;&#24230;&#21644;&#20301;&#32622;&#21453;&#39304;&#12290;"),
    (PXml -InnerXml "5. &#24403;&#21069; rot &#22312;&#19994;&#21153;&#38142;&#36335;&#19979;&#20351;&#29992;&#36895;&#24230;&#27169;&#24335; 3&#65292;&#30446;&#26631;&#36895;&#24230;&#30001; A &#26680;&#36890;&#36807; main.py &#25345;&#32493;&#19979;&#21457;&#12290;"),

    (PXml -InnerXml "&#22235;&#12289;&#26412;&#36718;&#37325;&#28857;&#38382;&#39064;&#19982;&#20462;&#22797;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#38382;&#39064;&#19968;&#65306;reboot &#21518;&#39537;&#21160;&#22120;&#20250;&#20986;&#29616;&#20004;&#27425;&#21169;&#30913;&#12290;"),
    (PXml -InnerXml "&#21407;&#22240;&#65306;&#19981;&#26159;&#39537;&#21160;&#22120;&#33258;&#36523;&#37325;&#22797;&#21169;&#30913;&#65292;&#32780;&#26159;&#33258;&#21160;&#21270;&#33050;&#26412;&#22312; reboot &#21518;&#21457;&#29983;&#20102;&#8220;systemd &#33258;&#21551;&#19968;&#27425; + &#33050;&#26412;&#25163;&#24037;&#20877;&#21551;&#21160;&#19968;&#27425;&#8221;&#30340;&#37325;&#22797;&#25289;&#36215;&#12290;&#31532;&#19968;&#27425;&#30001; robot-runner.service &#33258;&#21160;&#21551;&#21160; main.py&#65292;&#31532;&#20108;&#27425;&#30001; build_deploy_run_robot_monitor_ddr_release.ps1 &#22312;&#30475;&#21040;&#20018;&#21475;&#37324;&#31243;&#30865;&#21518;&#21448;&#20027;&#21160;&#35843;&#29992; Start-RemoteRobot&#65292;&#23548;&#33268;&#39537;&#21160;&#22120;&#37325;&#26032;&#36208;&#20102;&#19968;&#36941; 0x86 -&gt; 0x06 -&gt; 0x07 -&gt; 0x0F &#20351;&#33021;&#27969;&#31243;&#12290;"),
    (PXml -InnerXml "&#20462;&#22797;&#65306;&#20027;&#33050;&#26412;&#24050;&#22686;&#21152;&#26381;&#21153;&#21644;&#36827;&#31243;&#21028;&#23450;&#12290;&#21482;&#35201; robot-runner.service &#24050;&#32463;&#26159; active&#65292;&#25110;&#32773; /home/run_robot_remote.sh&#12289;python3 /home/main.py &#24050;&#32463;&#22312;&#36816;&#34892;&#65292;&#33050;&#26412;&#23601;&#19981;&#20877; pkill &#24182;&#37325;&#21551;&#65292;&#20174;&#32780;&#35299;&#20915;&#20102;&#37325;&#22797;&#21169;&#30913;&#38382;&#39064;&#12290;"),
    (PXml -InnerXml "&#38382;&#39064;&#20108;&#65306;reboot &#21518;&#30475;&#36215;&#26469;&#36824;&#27809;&#31561;&#21040; main.py&#65292;&#39537;&#21160;&#22120;&#23601;&#24050;&#32463;&#36827;&#20837;&#36895;&#24230;&#27169;&#24335;&#12290;"),
    (PXml -InnerXml "&#21407;&#22240;&#65306;&#36825;&#24182;&#19981;&#26159; M7 &#22266;&#20214;&#32469;&#36807; A &#26680;&#20599;&#20599;&#21551;&#21160;&#30340;&#38544;&#34255;&#36335;&#24452;&#65292;&#32780;&#26159;&#22240;&#20026;&#24403;&#21069;&#31995;&#32479;&#35774;&#35745;&#23601;&#26159;&#24320;&#26426;&#33258;&#21551; main.py&#12290;Linux &#20391;&#37325;&#21551;&#23436;&#25104;&#21518;&#65292;systemd &#20250;&#24456;&#24555;&#25289;&#36215; run_robot_remote.sh &#21644; main.py&#65292;&#25152;&#20197;&#20174;&#29616;&#35937;&#19978;&#30475;&#20687;&#26159;&#8220;&#27809;&#31561; main.py &#23601;&#24050;&#32463;&#36305;&#36215;&#26469;&#8221;&#65292;&#20294;&#23454;&#38469;&#19978; main.py &#24050;&#32463;&#22312;&#21518;&#21488;&#25191;&#34892;&#12290;"),
    (PXml -InnerXml "&#38382;&#39064;&#19977;&#65306;&#26089;&#26399;&#20960;&#36718;&#35843;&#35797;&#20013;&#26377;&#26102;&#20250;&#30475;&#21040;&#20018;&#21475;&#27491;&#24120;&#65292;&#20294;&#33258;&#21160;&#21270;&#33050;&#26412;&#20250;&#35823;&#25253;&#22833;&#36133;&#12290;"),
    (PXml -InnerXml "&#21407;&#22240;&#65306;reboot &#21518; SSH &#24674;&#22797;&#26102;&#38388;&#23384;&#22312;&#27874;&#21160;&#65292;&#21478;&#22806;&#26089;&#26399;&#33050;&#26412;&#25226;&#37096;&#20998;&#22266;&#20214;&#20018;&#21475;&#36755;&#20986;&#36807;&#26089;&#24403;&#25104;&#20102;&#8220;&#26426;&#22120;&#20154;&#31243;&#24207;&#21551;&#21160;&#25104;&#21151;&#8221;&#30340;&#26631;&#24535;&#12290;&#29616;&#22312;&#24050;&#32463;&#25910;&#25947;&#25104;&#26356;&#21487;&#38752;&#30340;&#25104;&#21151;&#21028;&#23450;&#26465;&#20214;&#65292;&#24182;&#22686;&#21152; /home/robot_runtime.log &#20316;&#20026;&#34917;&#20805;&#39564;&#35777;&#20381;&#25454;&#12290;"),

    (PXml -InnerXml "&#20116;&#12289;&#26412;&#27425;&#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#27979;&#35797;&#32467;&#26524;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "&#26412;&#27425;&#24050;&#37325;&#26032;&#25191;&#34892;&#23436;&#25972;&#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#38142;&#36335;&#65292;&#20027;&#20837;&#21475; bat &#25191;&#34892;&#36820;&#22238;&#30721;&#20026; 0&#65292;&#35828;&#26126;&#32534;&#35793;&#12289;&#37096;&#32626;&#12289;&#37325;&#21551;&#12289;&#33258;&#21551;&#19982;&#20018;&#21475;&#37319;&#38598;&#27969;&#31243;&#25972;&#20307;&#25104;&#21151;&#12290;"),
    (PXml -InnerXml "&#26368;&#26032;&#20018;&#21475;&#26085;&#24535;&#25991;&#20214;&#20026;&#65306;" -Bold $true),
    (PXml -InnerXml "D:\&#26700;&#38754;\imxSoem-motion-control\imxSoem-motion-control\mix_protocol\armgcc\serial_logs\robot_run_20260401_103944.log"),
    (PXml -InnerXml "&#26412;&#27425;&#26085;&#24535;&#32479;&#35745;&#32467;&#26524;&#22914;&#19979;&#65306;"),
    (PXml -InnerXml "1. ROT write control_mode=134 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "2. ROT write control_mode=6 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "3. ROT write control_mode=7 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "4. ROT write control_mode=15 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "5. ServoCan target speed latched: 3000000 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "6. RPDO1 sent: mode=0x03 ctrl=0x000F speed=3000000 &#20986;&#29616; 1 &#27425;&#12290;"),
    (PXml -InnerXml "7. Runtime feedback: modeDisplay=3 &#20849;&#20986;&#29616; 72 &#27425;&#12290;"),
    (PXml -InnerXml "&#32467;&#21512;&#26085;&#24535;&#20869;&#23481;&#21487;&#20197;&#30830;&#35748;&#65306;&#26412;&#36718;&#19981;&#20877;&#23384;&#22312;&#37325;&#22797;&#21169;&#30913;&#65292;rot &#22312;&#36895;&#24230;&#27169;&#24335; 3 &#19979;&#25345;&#32493;&#36816;&#34892;&#65292;actualSpeed &#38271;&#26102;&#38388;&#20445;&#25345;&#22312;&#32422; 2.8M &#21040; 3.1M &#38468;&#36817;&#65292;actualPosition &#25345;&#32493;&#21464;&#21270;&#65292;&#21487;&#21028;&#23450;&#30005;&#26426;&#22312;&#25345;&#32493;&#36716;&#21160;&#12290;"),

    (PXml -InnerXml "&#20845;&#12289;&#24403;&#21069;&#29256;&#26412;&#30340;&#20351;&#29992;&#26041;&#24335;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. &#38656;&#35201;&#37325;&#32534;&#35793;&#12289;&#37096;&#32626;&#24182;&#39564;&#35777;&#26102;&#65292;&#30452;&#25509;&#21452;&#20987;&#20027;&#20837;&#21475; bat &#21363;&#21487;&#65292;&#19981;&#38656;&#35201;&#20877;&#20351;&#29992;&#20854;&#20182;&#24555;&#36895;&#29256;&#25110;&#20020;&#26102;&#33050;&#26412;&#12290;"),
    (PXml -InnerXml "2. &#24320;&#21457;&#26495; reboot &#21518;&#65292;Linux &#20391;&#20250;&#30001; robot-runner.service &#33258;&#21160;&#21551;&#21160; main.py&#65292;&#25152;&#20197;&#21482;&#35201;&#26381;&#21153;&#20445;&#25345;&#21551;&#29992;&#29366;&#24577;&#65292;&#30005;&#26426;&#23601;&#20250;&#22312;&#31995;&#32479;&#21551;&#21160;&#23436;&#25104;&#21518;&#33258;&#21160;&#25353;&#24403;&#21069; main.py &#35774;&#23450;&#30340;&#36895;&#24230;&#27169;&#24335;&#21644;&#30446;&#26631;&#36895;&#24230;&#36816;&#34892;&#12290;"),
    (PXml -InnerXml "3. &#22914;&#26524;&#21518;&#32493;&#24076;&#26395;&#25913;&#25104;&#8220;&#24320;&#26426;&#38745;&#27490;&#65292;&#31561;&#20154;&#24037;&#30830;&#35748;&#21518;&#20877;&#36816;&#34892;&#8221;&#30340;&#26041;&#24335;&#65292;&#24212;&#20248;&#20808;&#35843;&#25972; systemd &#33258;&#21551;&#31574;&#30053;&#65292;&#32780;&#19981;&#26159;&#20165;&#20462;&#25913;&#20018;&#21475;&#30417;&#21548;&#36923;&#36753;&#12290;"),

    (PXml -InnerXml "&#19971;&#12289;&#32467;&#35770;" -Bold $true -Size 28 -Font "Microsoft YaHei"),
    (PXml -InnerXml "1. &#24403;&#21069; mix_protocol &#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#38142;&#36335;&#24050;&#32463;&#21487;&#31283;&#23450;&#20351;&#29992;&#12290;"),
    (PXml -InnerXml "2. &#26412;&#27425;&#37325;&#28857;&#38382;&#39064;&#20013;&#65292;&#37325;&#22797;&#21169;&#30913;&#26159;&#33050;&#26412;&#37325;&#22797;&#25289;&#36215;&#23548;&#33268;&#65292;&#29616;&#24050;&#20462;&#22797;&#12290;"),
    (PXml -InnerXml "3. reboot &#21518;&#24456;&#24555;&#36827;&#20837;&#36895;&#24230;&#27169;&#24335;&#30340;&#29616;&#35937;&#65292;&#26159;&#24403;&#21069;&#24320;&#26426;&#33258;&#21551; main.py &#35774;&#35745;&#30340;&#30452;&#25509;&#32467;&#26524;&#65292;&#23646;&#20110;&#24403;&#21069;&#26041;&#26696;&#30340;&#39044;&#26399;&#34892;&#20026;&#12290;"),
    (PXml -InnerXml "4. &#22914;&#38656;&#32487;&#32493;&#20132;&#25509;&#25110;&#25193;&#23637;&#65292;&#24314;&#35758;&#21516;&#26102;&#20445;&#30041;&#26412;&#35828;&#26126;&#25991;&#26723;&#12289;&#20027;&#20837;&#21475; bat &#25991;&#20214;&#21644;&#26368;&#26032;&#20018;&#21475;&#26085;&#24535;&#65292;&#26041;&#20415;&#21518;&#32493;&#20154;&#21592;&#24555;&#36895;&#22797;&#29616;&#12290;")
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
  <dc:title>mix_protocol 自动化链路说明</dc:title>
  <dc:creator>Codex</dc:creator>
  <cp:lastModifiedBy>Codex</cp:lastModifiedBy>
  <dcterms:created xsi:type="dcterms:W3CDTF">2026-04-01T04:35:00Z</dcterms:created>
  <dcterms:modified xsi:type="dcterms:W3CDTF">2026-04-01T04:35:00Z</dcterms:modified>
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
        "&#33258;&#21160;&#21270;&#38142;&#36335;&#35828;&#26126;",
        "&#24403;&#21069;&#33258;&#21160;&#21270;&#38142;&#36335;&#27010;&#36848;",
        "&#37325;&#22797;&#25289;&#36215;",
        "&#36895;&#24230;&#27169;&#24335; 3",
        "&#26412;&#27425;&#37325;&#32534;&#35793;&#33258;&#21160;&#21270;&#27979;&#35797;&#32467;&#26524;"
    )

    foreach ($check in $checks) {
        if ($xmlText -notmatch [regex]::Escape($check)) {
            throw "Self-check failed: missing expected entity text."
        }
    }
}
finally {
    $checkZip.Dispose()
}

Get-Item $outFile | Select-Object FullName, Length, LastWriteTime | Format-List
