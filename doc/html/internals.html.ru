<html><head><meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1"><title>&#1042;&#1085;&#1091;&#1090;&#1088;&#1077;&#1085;&#1085;&#1086;&#1089;&#1090;&#1080; Rivet</title><link rel="stylesheet" href="rivet.css" type="text/css"><meta name="generator" content="DocBook XSL Stylesheets V1.60.1"><link rel="home" href="index.html.ru" title="Apache Rivet"><link rel="up" href="index.html.ru" title="Apache Rivet"><link rel="previous" href="help.html.ru" title="&#1056;&#1077;&#1089;&#1091;&#1088;&#1089;&#1099; &#1080; &#1082;&#1072;&#1082; &#1087;&#1086;&#1083;&#1091;&#1095;&#1080;&#1090;&#1100; &#1087;&#1086;&#1084;&#1086;&#1097;&#1100;"><link rel="next" href="upgrading.html.ru" title="&#1055;&#1077;&#1088;&#1077;&#1093;&#1086;&#1076; &#1089;  mod_dtcl &#1080;&#1083;&#1080; NeoWebScript (NWS)"></head><body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF"><div class="navheader"><table width="100%" summary="Navigation header"><tr><th colspan="3" align="center">&#1042;&#1085;&#1091;&#1090;&#1088;&#1077;&#1085;&#1085;&#1086;&#1089;&#1090;&#1080; Rivet</th></tr><tr><td width="20%" align="left"><a accesskey="p" href="help.html.ru"><img src="images/prev.png" alt="Prev"></a> </td><th width="60%" align="center"> </th><td width="20%" align="right"> <a accesskey="n" href="upgrading.html.ru"><img src="images/next.png" alt="Next"></a></td></tr></table></div><div class="section" lang="en"><div class="titlepage"><div><div><hr><h2 class="title" style="clear: both"><a name="internals"></a>&#1042;&#1085;&#1091;&#1090;&#1088;&#1077;&#1085;&#1085;&#1086;&#1089;&#1090;&#1080; Rivet</h2></div></div><div></div></div><p style="width:90%">
      &#1069;&#1090;&#1072; &#1089;&#1077;&#1082;&#1094;&#1080;&#1103; &#1076;&#1072;&#1074;&#1085;&#1086; &#1091;&#1089;&#1090;&#1072;&#1088;&#1077;&#1083;&#1072;, &#1087;&#1086;&#1089;&#1082;&#1086;&#1083;&#1100;&#1082;&#1091; &#1085;&#1086;&#1074;&#1099;&#1081; &#1082;&#1086;&#1076; &#1076;&#1086;&#1073;&#1072;&#1074;&#1083;&#1103;&#1077;&#1090;&#1089;&#1103;, &#1072; 
      &#1089;&#1090;&#1072;&#1088;&#1099;&#1081; &#1091;&#1073;&#1080;&#1088;&#1072;&#1077;&#1090;&#1089;&#1103;. &#1058;&#1072;&#1082; &#1095;&#1090;&#1086; &#1083;&#1091;&#1095;&#1096;&#1077; &#1095;&#1080;&#1090;&#1072;&#1081;&#1090;&#1077; &#1080;&#1089;&#1093;&#1086;&#1076;&#1085;&#1080;&#1082;&#1080; - &#1086;&#1085;&#1080; &#1088;&#1091;&#1083;&#1077;&#1079;!.
      &#1048; &#1074;&#1086;&#1086;&#1073;&#1097;&#1077; &#1077;&#1089;&#1083;&#1080; &#1093;&#1086;&#1090;&#1080;&#1090;&#1077; &#1095;&#1077;&#1075;&#1086;-&#1085;&#1080;&#1073;&#1091;&#1076;&#1100; &#1085;&#1072;&#1087;&#1080;&#1089;&#1072;&#1090;&#1100; - FIXME.
    </p><div class="section" lang="en"><div class="titlepage"><div><div><h3 class="title"><a name="id5397082"></a>&#1048;&#1085;&#1080;&#1094;&#1080;&#1072;&#1083;&#1080;&#1079;&#1072;&#1094;&#1080;&#1103;</h3></div></div><div></div></div><p style="width:90%">
        &#1050;&#1086;&#1075;&#1076;&#1072; apavhe &#1079;&#1072;&#1087;&#1091;&#1089;&#1082;&#1072;&#1077;&#1090;&#1089;&#1103; (&#1080;&#1083;&#1080; &#1082;&#1086;&#1075;&#1076;&#1072; &#1079;&#1072;&#1087;&#1091;&#1089;&#1082;&#1072;&#1077;&#1090;&#1089;&#1103; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1091;&#1102;&#1097;&#1080;&#1081; TCL
        &#1076;&#1086;&#1095;&#1077;&#1088;&#1085;&#1080;&#1081; &#1087;&#1088;&#1086;&#1094;&#1077;&#1089;&#1089; Apache ), &#1074;&#1099;&#1079;&#1099;&#1074;&#1072;&#1077;&#1090;&#1089;&#1103;
        <tt class="function">Rivet_InitTclStuff</tt>,
        &#1082;&#1086;&#1090;&#1086;&#1088;&#1099;&#1081; &#1089;&#1086;&#1079;&#1076;&#1072;&#1077;&#1090; &#1085;&#1086;&#1074;&#1099;&#1081; &#1080;&#1085;&#1090;&#1077;&#1088;&#1087;&#1088;&#1077;&#1090;&#1072;&#1090;&#1086;&#1088;, &#1087;&#1086; &#1086;&#1076;&#1085;&#1086;&#1084;&#1091; &#1085;&#1072; &#1082;&#1072;&#1078;&#1076;&#1099;&#1081;
        &#1074;&#1080;&#1088;&#1090;&#1091;&#1072;&#1083;&#1100;&#1085;&#1099;&#1081; &#1093;&#1086;&#1089;&#1090;, &#1074; &#1079;&#1072;&#1074;&#1080;&#1089;&#1080;&#1084;&#1086;&#1089;&#1090;&#1080; &#1086;&#1090; &#1082;&#1086;&#1085;&#1092;&#1080;&#1075;&#1091;&#1088;&#1072;&#1094;&#1080;&#1080;. &#1058;&#1072;&#1082;&#1078;&#1077; &#1080;&#1085;&#1080;&#1094;&#1080;&#1072;&#1083;&#1080;&#1079;&#1080;&#1088;&#1091;&#1102;&#1090;&#1089;&#1103;
        &#1074;&#1089;&#1103;&#1082;&#1080;&#1077; &#1074;&#1077;&#1097;&#1080; &#1090;&#1080;&#1087;&#1072; 
        <span class="structname">RivetChan</span> &#1082;&#1072;&#1085;&#1072;&#1083;&#1100;&#1085;&#1072;&#1103; &#1089;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; (channel system),
        &#1089;&#1086;&#1079;&#1076;&#1072;&#1102;&#1090;&#1089;&#1103; &#1089;&#1087;&#1077;&#1094;&#1080;&#1092;&#1080;&#1095;&#1085;&#1099;&#1077; &#1076;&#1083;&#1103; Rivet Tcl &#1082;&#1086;&#1084;&#1072;&#1085;&#1076;&#1099; &#1080; &#1080;&#1089;&#1087;&#1086;&#1083;&#1085;&#1103;&#1077;&#1090;&#1089;&#1103; Rivet&#1086;&#1074;&#1089;&#1082;&#1080;&#1081; 
        channel system, creates the Rivet-specific Tcl commands, and
        <tt class="filename">init.tcl</tt>.  &#1057;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; &#1082;&#1077;&#1096;&#1080;&#1088;&#1086;&#1074;&#1072;&#1085;&#1080;&#1103;, &#1086;&#1087;&#1103;&#1090;&#1100; &#1078;&#1077;, &#1080; &#1077;&#1089;&#1083;&#1080;
        &#1077;&#1089;&#1090;&#1100;
        <span style="font-family:monospace"><b class="command">GlobalInitScript</b></span>, &#1090;&#1086; &#1080; &#1086;&#1085; &#1079;&#1072;&#1087;&#1091;&#1089;&#1082;&#1072;&#1077;&#1090;&#1089;&#1103;.
      </p></div><div class="section" lang="en"><div class="titlepage"><div><div><h3 class="title"><a name="id5397143"></a>RivetChan</h3></div></div><div></div></div><p style="width:90%">
        &#1057;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; <span class="structname">RivetChan</span> &#1073;&#1099;&#1083;&#1072; &#1089;&#1086;&#1079;&#1076;&#1072;&#1085;&#1072; &#1076;&#1083;&#1103; &#1090;&#1086;&#1075;&#1086; &#1095;&#1090;&#1086;&#1073;&#1099;
        &#1087;&#1077;&#1088;&#1077;&#1085;&#1072;&#1087;&#1088;&#1072;&#1074;&#1083;&#1103;&#1090;&#1100; &#1089;&#1090;&#1072;&#1085;&#1076;&#1072;&#1088;&#1090;&#1085;&#1099;&#1081; &#1087;&#1086;&#1090;&#1086;&#1082; &#1074;&#1099;&#1074;&#1086;&#1076;&#1072; (stdout). &#1055;&#1086; &#1101;&#1090;&#1086;&#1081; &#1087;&#1088;&#1080;&#1095;&#1080;&#1085;&#1077; &#1074;&#1099; &#1084;&#1086;&#1078;&#1077;&#1090;&#1077;
        &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1086;&#1074;&#1072;&#1090;&#1100; &#1089;&#1090;&#1072;&#1085;&#1076;&#1072;&#1088;&#1090;&#1085;&#1091;&#1102; &#1082;&#1086;&#1084;&#1072;&#1085;&#1076;&#1091; 
        <span style="font-family:monospace"><b class="command">puts</b></span> &#1074; &#1089;&#1090;&#1088;&#1072;&#1085;&#1080;&#1094;&#1072;&#1093; .rvt.
        (&#1087;&#1088;&#1080;&#1084;&#1077;&#1095;&#1072;&#1085;&#1080;&#1077; &#1087;&#1077;&#1088;&#1077;&#1074;&#1086;&#1076;&#1095;&#1080;&#1082;&#1072;: &#1080; <span style="font-family:monospace"><b class="command">fconfigure</b></span> &#1090;&#1086;&#1078;&#1077;, &#1095;&#1090;&#1086; &#1087;&#1086;&#1083;&#1077;&#1079;&#1085;&#1086;).
        &#1054;&#1085;&#1072; &#1090;&#1072;&#1082;&#1078;&#1077; &#1089;&#1086;&#1079;&#1076;&#1072;&#1077;&#1090; &#1082;&#1072;&#1085;&#1072;&#1083; &#1082;&#1086;&#1090;&#1086;&#1088;&#1099;&#1081; &#1073;&#1091;&#1092;&#1077;&#1088;&#1080;&#1079;&#1091;&#1077;&#1090; output, &#1080; &#1087;&#1077;&#1088;&#1077;&#1085;&#1072;&#1087;&#1088;&#1072;&#1074;&#1083;&#1103;&#1077;&#1090; &#1077;&#1075;&#1086; &#1074; 
        &#1089;&#1080;&#1089;&#1090;&#1077;&#1084;&#1091; &#1074;&#1074;&#1086;&#1076;&#1072;-&#1074;&#1099;&#1074;&#1086;&#1076;&#1072; Apache. 
      </p></div><div class="section" lang="en"><div class="titlepage"><div><div><h3 class="title"><a name="id5397190"></a>&#1054;&#1073;&#1088;&#1072;&#1073;&#1086;&#1090;&#1082;&#1072; &#1089;&#1090;&#1088;&#1072;&#1085;&#1080;&#1094;, &#1074;&#1099;&#1087;&#1086;&#1083;&#1085;&#1077;&#1085;&#1080;&#1077; &#1080; &#1082;&#1077;&#1096;&#1080;&#1088;&#1086;&#1074;&#1072;&#1085;&#1080;&#1077;</h3></div></div><div></div></div><p style="width:90%">
        &#1050;&#1086;&#1075;&#1076;&#1072; &#1074;&#1099;&#1087;&#1086;&#1083;&#1085;&#1103;&#1077;&#1090;&#1089;&#1103; Rivet &#1089;&#1090;&#1088;&#1072;&#1085;&#1080;&#1094;&#1072;, &#1086;&#1085;&#1072; &#1087;&#1088;&#1077;&#1074;&#1088;&#1072;&#1097;&#1072;&#1077;&#1090;&#1089;&#1103; &#1074; &#1086;&#1073;&#1099;&#1095;&#1085;&#1099;&#1081; Tcl
        &#1089;&#1082;&#1088;&#1080;&#1087;&#1090;, &#1086;&#1087;&#1080;&#1088;&#1072;&#1103;&#1089;&#1100; &#1085;&#1072; &#1089;&#1080;&#1084;&#1074;&#1086;&#1083;&#1099;  &lt;? ?&gt;. &#1042;&#1089;&#1077; &#1095;&#1090;&#1086; &#1074;&#1085;&#1077; &#1080;&#1093; 
        &#1088;&#1072;&#1089;&#1089;&#1084;&#1072;&#1090;&#1088;&#1080;&#1074;&#1072;&#1077;&#1090;&#1089;&#1103; &#1082;&#1072;&#1082; &#1073;&#1086;&#1083;&#1100;&#1096;&#1086;&#1081; &#1073;&#1086;&#1083;&#1100;&#1096;&#1086;&#1081; &#1072;&#1088;&#1075;&#1091;&#1084;&#1077;&#1085;&#1090; &#1076;&#1083;&#1103;
        <span style="font-family:monospace"><b class="command">puts</b></span>, &#1072; &#1074;&#1089;&#1077; &#1095;&#1090;&#1086; &#1074;&#1085;&#1091;&#1090;&#1088;&#1080; - &#1088;&#1072;&#1089;&#1089;&#1084;&#1072;&#1090;&#1088;&#1080;&#1074;&#1072;&#1077;&#1090;&#1089;&#1103; 
        &#1082;&#1072;&#1082; Tcl &#1082;&#1086;&#1076;.
      </p><p style="width:90%">
        &#1050;&#1072;&#1078;&#1076;&#1099;&#1081; .rvt &#1092;&#1072;&#1081;&#1083; &#1074;&#1099;&#1087;&#1086;&#1083;&#1085;&#1103;&#1077;&#1090;&#1089;&#1103; &#1074; &#1087;&#1088;&#1086;&#1089;&#1090;&#1072;&#1085;&#1089;&#1090;&#1074;&#1077; &#1080;&#1084;&#1077;&#1085;
        <tt class="constant">::request</tt>, &#1087;&#1086; &#1101;&#1090;&#1086;&#1084;&#1091; &#1085;&#1077;&#1090; &#1085;&#1077;&#1086;&#1073;&#1093;&#1086;&#1076;&#1080;&#1084;&#1086;&#1089;&#1090;&#1080;
        &#1082;&#1072;&#1078;&#1076;&#1099;&#1081; &#1088;&#1072;&#1079; &#1089;&#1086;&#1079;&#1076;&#1072;&#1074;&#1072;&#1090;&#1100; &#1087;&#1086; &#1085;&#1086;&#1074;&#1086;&#1084;&#1091; &#1080;&#1085;&#1090;&#1077;&#1088;&#1087;&#1088;&#1077;&#1090;&#1072;&#1090;&#1086;&#1088;&#1091;. &#1055;&#1086; &#1101;&#1090;&#1086;&#1081; &#1078;&#1077;
        &#1087;&#1088;&#1080;&#1095;&#1080;&#1085;&#1077; &#1075;&#1083;&#1086;&#1073;&#1072;&#1083;&#1100;&#1085;&#1099;&#1077; &#1087;&#1077;&#1088;&#1077;&#1084;&#1077;&#1085;&#1085;&#1099;&#1077; 
        (&#1087;&#1088;&#1080;&#1084;&#1077;&#1095;&#1072;&#1085;&#1080;&#1077; &#1087;&#1088;&#1077;&#1074;&#1086;&#1076;&#1095;&#1080;&#1082;&#1072;: &#1079;&#1074;&#1080;&#1085;&#1103;&#1081;&#1090;&#1077;, &#1085;&#1077; &#1087;&#1086;&#1085;&#1103;&#1083;)
        By running in its own namespace, though, each page will
        not run afoul of local variables created by other scripts,
        because they will be deleted automatically when the namespace
        goes away after Apache finishes handling the request.
      </p><div class="note" style="margin-left: 0.5in; margin-right: 0.5in;"><table border="0" summary="Note"><tr><td rowspan="2" align="center" valign="top" width="25"><img alt="[Note]" src="images/note.png"></td><th align="left">Note</th></tr><tr><td colspan="2" align="left" valign="top">
        &#1054;&#1076;&#1085;&#1072; &#1080;&#1079; &#1089;&#1091;&#1097;&#1077;&#1089;&#1090;&#1074;&#1091;&#1102;&#1097;&#1080;&#1093; &#1085;&#1099;&#1085;&#1077; &#1087;&#1088;&#1086;&#1073;&#1083;&#1077;&#1084; &#1087;&#1086;&#1076;&#1086;&#1073;&#1085;&#1086;&#1075;&#1086; &#1087;&#1086;&#1076;&#1093;&#1086;&#1076;&#1072; - &#1101;&#1090;&#1086; &#1086;&#1090;&#1089;&#1091;&#1090;&#1089;&#1074;&#1080;&#1077;
        &#1084;&#1077;&#1093;&#1072;&#1085;&#1080;&#1079;&#1084;&#1072; &#1089;&#1073;&#1086;&#1088;&#1097;&#1080;&#1082;&#1072; &#1084;&#1091;&#1089;&#1086;&#1088;&#1072;, &#1085;&#1072;&#1087;&#1088;&#1080;&#1084;&#1077;&#1088;, &#1077;&#1089;&#1083;&#1080; &#1074;&#1099; &#1085;&#1077; &#1079;&#1072;&#1082;&#1088;&#1099;&#1083;&#1080; 
        &#1093;&#1101;&#1085;&#1076;&#1083; &#1092;&#1072;&#1081;&#1083;&#1072; - &#1090;&#1086; &#1101;&#1090;&#1086; &#1087;&#1083;&#1086;&#1093;&#1086;. &#1055;&#1086;&#1101;&#1090;&#1086;&#1084;&#1091; &#1073;&#1091;&#1076;&#1100;&#1090;&#1077; &#1076;&#1086;&#1073;&#1088;&#1099;, &#1079;&#1072;&#1082;&#1088;&#1099;&#1074;&#1072;&#1081;&#1090;&#1077; &#1074;&#1089;&#1077;
        &#1095;&#1090;&#1086; &#1085;&#1072;&#1086;&#1090;&#1082;&#1088;&#1099;&#1074;&#1072;&#1083;&#1080;.
      </td></tr></table></div><p style="width:90%">
        &#1055;&#1086;&#1089;&#1083;&#1077; &#1090;&#1086;&#1075;&#1086; &#1082;&#1072;&#1082; &#1089;&#1082;&#1088;&#1080;&#1087;&#1090; &#1079;&#1072;&#1075;&#1088;&#1091;&#1078;&#1077;&#1085; &#1080; &#1087;&#1088;&#1077;&#1086;&#1073;&#1088;&#1072;&#1079;&#1086;&#1074;&#1072;&#1085; &#1074; &quot;&#1095;&#1080;&#1089;&#1090;&#1099;&#1081; Tcl&quot;,
        &#1086;&#1085; &#1086;&#1087;&#1103;&#1090;&#1100; &#1078;&#1077; &#1082;&#1077;&#1096;&#1080;&#1088;&#1091;&#1077;&#1090;&#1089;&#1103;, &#1080; &#1087;&#1086; &#1101;&#1090;&#1086;&#1081; &#1087;&#1088;&#1080;&#1095;&#1080;&#1085;&#1077; &#1084;&#1086;&#1078;&#1077;&#1090; &#1073;&#1099;&#1090;&#1100; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1086;&#1074;&#1072;&#1085; 
        &#1074; &#1089;&#1083;&#1077;&#1076;&#1091;&#1102;&#1097;&#1080;&#1081; &#1088;&#1072;&#1079; &#1073;&#1077;&#1079; &#1083;&#1080;&#1096;&#1085;&#1080;&#1093; &#1087;&#1088;&#1077;&#1086;&#1073;&#1088;&#1072;&#1079;&#1086;&#1074;&#1072;&#1085;&#1080;&#1081; &#1080; &#1086;&#1073;&#1088;&#1072;&#1097;&#1077;&#1085;&#1080;&#1081; &#1082; &#1076;&#1080;&#1089;&#1082;&#1091;.
        &#1050;&#1086;&#1083;&#1080;&#1095;&#1077;&#1089;&#1090;&#1074;&#1086; &#1082;&#1077;&#1096;&#1080;&#1088;&#1091;&#1077;&#1084;&#1099;&#1093; &#1089;&#1082;&#1088;&#1080;&#1087;&#1090;&#1086;&#1074; &#1084;&#1086;&#1078;&#1085;&#1086; &#1080;&#1079;&#1084;&#1077;&#1085;&#1080;&#1090;&#1100; &#1074; &#1082;&#1086;&#1085;&#1092;&#1080;&#1075;&#1077;. 
        &#1058;&#1072;&#1082;&#1080;&#1084; &#1086;&#1073;&#1088;&#1072;&#1079;&#1086;&#1084; &#1084;&#1086;&#1078;&#1085;&#1086; &#1079;&#1085;&#1072;&#1095;&#1080;&#1090;&#1077;&#1083;&#1100;&#1085;&#1086; &#1087;&#1086;&#1076;&#1085;&#1103;&#1090;&#1100; &#1087;&#1088;&#1086;&#1080;&#1079;&#1074;&#1086;&#1076;&#1080;&#1090;&#1077;&#1083;&#1100;&#1085;&#1086;&#1089;&#1090;&#1100; &#1089;&#1080;&#1089;&#1090;&#1077;&#1084;&#1099;.
      </p></div></div><div class="navfooter"><hr><table width="100%" summary="Navigation footer"><tr><td width="40%" align="left"><a accesskey="p" href="help.html.ru"><img src="images/prev.png" alt="Prev"></a> </td><td width="20%" align="center"><a accesskey="u" href="index.html.ru"><img src="images/up.png" alt="Up"></a></td><td width="40%" align="right"> <a accesskey="n" href="upgrading.html.ru"><img src="images/next.png" alt="Next"></a></td></tr><tr><td width="40%" align="left" valign="top">&#1056;&#1077;&#1089;&#1091;&#1088;&#1089;&#1099; &#1080; &#1082;&#1072;&#1082; &#1087;&#1086;&#1083;&#1091;&#1095;&#1080;&#1090;&#1100; &#1087;&#1086;&#1084;&#1086;&#1097;&#1100; </td><td width="20%" align="center"><a accesskey="h" href="index.html.ru"><img src="images/home.png" alt="Home"></a></td><td width="40%" align="right" valign="top"> &#1055;&#1077;&#1088;&#1077;&#1093;&#1086;&#1076; &#1089;  mod_dtcl &#1080;&#1083;&#1080; NeoWebScript (NWS)</td></tr></table></div></body></html>
