<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html><head><meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1"><title>Apache Rivet</title><link rel="stylesheet" href="rivet.css" type="text/css"><meta name="generator" content="DocBook XSL Stylesheets V1.60.1"><link rel="home" href="index.html.ru" title="Apache Rivet"><link rel="next" href="installation.html.ru" title="&#1059;&#1089;&#1090;&#1072;&#1085;&#1086;&#1074;&#1082;&#1072; Apache Rivet"></head><body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF"><div class="navheader"><table width="100%" summary="Navigation header"><tr><th colspan="3" align="center">Apache Rivet</th></tr><tr><td width="20%" align="left">�</td><th width="60%" align="center">�</th><td width="20%" align="right">�<a accesskey="n" href="installation.html.ru"><img src="images/next.png" alt="Next"></a></td></tr></table></div><div class="article" lang="en"><div class="titlepage"><div><div><h1 class="title"><a name="id5336302"></a>Apache Rivet</h1></div><div><div class="author"><h3 class="author"><span class="firstname">David</span> <span class="surname">Welton</span></h3><div class="affiliation"><div class="address"><p><br>
����������<tt class="email">&lt;<a href="mailto:davidw@apache.org">davidw@apache.org</a>&gt;</tt><br>
��������</p></div></div></div></div><div><div class="author"><h3 class="author"><span class="firstname">Damon</span> <span class="surname">Courtney</span></h3><div class="affiliation"><div class="address"><p><br>
����������<tt class="email">&lt;<a href="mailto:damonc@apache.org">damonc@apache.org</a>&gt;</tt><br>
��������</p></div></div></div></div><div><p class="othercredit"><span class="contrib">Russian Translation</span>: <span class="firstname">Dmitry</span> <span class="othername">the</span> <span class="surname">Zuryanovich</span></p><div class="affiliation"><div class="address"><p><tt class="email">&lt;<a href="mailto:dtz@xepb.ru">dtz@xepb.ru</a>&gt;</tt></p></div></div></div><div><p class="copyright">Copyright � 2002, 2003 Apache Software Foundation</p></div></div><div></div><hr></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><a href="index.html.ru#id5393393">&#1042;&#1074;&#1077;&#1076;&#1077;&#1085;&#1080;&#1077; &#1074;  Apache Rivet</a></dt><dt><a href="installation.html.ru">&#1059;&#1089;&#1090;&#1072;&#1085;&#1086;&#1074;&#1082;&#1072; Apache Rivet</a></dt><dt><a href="directives.html.ru">&#1050;&#1086;&#1085;&#1092;&#1080;&#1075;&#1091;&#1088;&#1072;&#1094;&#1080;&#1086;&#1085;&#1085;&#1099;&#1077; &#1087;&#1072;&#1088;&#1072;&#1084;&#1077;&#1090;&#1088;&#1099; Rivet Apache</a></dt><dt><a href="commands.html.ru">Rivet Tcl: &#1082;&#1086;&#1084;&#1072;&#1085;&#1076;&#1099; &#1080; &#1087;&#1077;&#1088;&#1077;&#1084;&#1077;&#1085;&#1085;&#1099;&#1077;</a></dt><dt><a href="examples.html.ru">&#1055;&#1088;&#1080;&#1084;&#1077;&#1088;&#1099; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1086;&#1074;&#1072;&#1085;&#1080;&#1103;</a></dt><dt><a href="help.html.ru">&#1056;&#1077;&#1089;&#1091;&#1088;&#1089;&#1099; &#1080; &#1082;&#1072;&#1082; &#1087;&#1086;&#1083;&#1091;&#1095;&#1080;&#1090;&#1100; &#1087;&#1086;&#1084;&#1086;&#1097;&#1100;</a></dt><dd><dl><dt><a href="help.html.ru#id5397392">Mailing Lists</a></dt><dt><a href="help.html.ru#websites">Web Sites</a></dt><dt><a href="help.html.ru#id5396420">&#1057;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; &#1086;&#1090;&#1083;&#1086;&#1074;&#1072; &#1086;&#1096;&#1080;&#1073;&#1086;&#1082;</a></dt></dl></dd><dt><a href="internals.html.ru">&#1042;&#1085;&#1091;&#1090;&#1088;&#1077;&#1085;&#1085;&#1086;&#1089;&#1090;&#1080; Rivet</a></dt><dd><dl><dt><a href="internals.html.ru#id5396479">&#1048;&#1085;&#1080;&#1094;&#1080;&#1072;&#1083;&#1080;&#1079;&#1072;&#1094;&#1080;&#1103;</a></dt><dt><a href="internals.html.ru#id5397057">RivetChan</a></dt><dt><a href="internals.html.ru#id5397105">&#1054;&#1073;&#1088;&#1072;&#1073;&#1086;&#1090;&#1082;&#1072; &#1089;&#1090;&#1088;&#1072;&#1085;&#1080;&#1094;, &#1074;&#1099;&#1087;&#1086;&#1083;&#1085;&#1077;&#1085;&#1080;&#1077; &#1080; &#1082;&#1077;&#1096;&#1080;&#1088;&#1086;&#1074;&#1072;&#1085;&#1080;&#1077;</a></dt></dl></dd><dt><a href="upgrading.html.ru">&#1055;&#1077;&#1088;&#1077;&#1093;&#1086;&#1076; &#1089;  mod_dtcl &#1080;&#1083;&#1080; NeoWebScript (NWS)</a></dt><dd><dl><dt><a href="upgrading.html.ru#id5397230">mod_dtcl</a></dt><dt><a href="upgrading.html.ru#id5397253">NeoWebScript</a></dt></dl></dd></dl></div><p style="width:90%">
    This document is also available in the following languages: <a href="index.html.en" target="_top">English</a>
  </p><p style="width:90%">This document is based on version 1.20 of the original English
  version.</p><div class="section" lang="en"><div class="titlepage"><div><div><hr><h2 class="title" style="clear: both"><a name="id5393393"></a>&#1042;&#1074;&#1077;&#1076;&#1077;&#1085;&#1080;&#1077; &#1074;  Apache Rivet</h2></div></div><div></div></div><p style="width:90%">
      Apache Rivet - &#1101;&#1090;&#1086; &#1089;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; &#1076;&#1083;&#1103; &#1075;&#1077;&#1085;&#1077;&#1088;&#1072;&#1094;&#1080;&#1080; &#1076;&#1080;&#1085;&#1072;&#1084;&#1080;&#1095;&#1077;&#1089;&#1082;&#1080;&#1093; &#1074;&#1077;&#1073;
      &#1089;&#1090;&#1088;&#1072;&#1085;&#1080;&#1094;, &#1074;&#1089;&#1090;&#1088;&#1086;&#1077;&#1085;&#1085;&#1072;&#1103; &#1084;&#1086;&#1076;&#1091;&#1083;&#1077;&#1084; &#1074; http &#1089;&#1077;&#1088;&#1074;&#1077;&#1088; Apache. &#1054;&#1085;&#1072; &#1079;&#1072;&#1076;&#1091;&#1084;&#1099;&#1074;&#1072;&#1083;&#1072;&#1089;&#1100; 
      &#1082;&#1072;&#1082; &#1089;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072; &#1073;&#1099;&#1089;&#1090;&#1088;&#1072;&#1103;, &#1084;&#1086;&#1097;&#1085;&#1072;&#1103;, &#1088;&#1072;&#1089;&#1096;&#1080;&#1088;&#1103;&#1077;&#1084;&#1072;&#1103; &#1080; &#1076;&#1086;&#1089;&#1090;&#1072;&#1090;&#1086;&#1095;&#1085;&#1086; &#1087;&#1088;&#1086;&#1089;&#1090;&#1072;&#1103; &#1076;&#1083;&#1103;
      &#1074;&#1085;&#1077;&#1076;&#1088;&#1077;&#1085;&#1080;&#1103;. &#1057;&#1090;&#1086;&#1080;&#1090; &#1079;&#1072;&#1084;&#1077;&#1090;&#1080;&#1090;&#1100; &#1095;&#1090;&#1086; &#1089; Apache Rivet &#1074;&#1099; &#1087;&#1086;&#1083;&#1091;&#1095;&#1072;&#1077;&#1090;&#1077; &#1085;&#1072;&#1076;&#1077;&#1078;&#1085;&#1091;&#1102;
      &#1087;&#1083;&#1072;&#1090;&#1092;&#1086;&#1088;&#1084;&#1091; &#1082;&#1086;&#1090;&#1086;&#1088;&#1072;&#1103; &#1085;&#1077; &#1090;&#1086;&#1083;&#1100;&#1082;&#1086; &#1084;&#1086;&#1078;&#1077;&#1090; &#1080;&#1085;&#1090;&#1077;&#1075;&#1088;&#1080;&#1088;&#1086;&#1074;&#1072;&#1090;&#1100;&#1089;&#1103; &#1074; web, &#1085;&#1086;
      &#1080; &#1084;&#1086;&#1078;&#1077;&#1090; &#1073;&#1099;&#1090;&#1100; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1086;&#1074;&#1072;&#1085;&#1072; &#1074;&#1085;&#1077; &#1077;&#1075;&#1086;. GUI &#1080; &#1087;&#1088;&#1080;&#1083;&#1086;&#1078;&#1077;&#1085;&#1080;&#1103; &#1082;&#1086;&#1084;&#1072;&#1085;&#1076;&#1085;&#1086;&#1081; 
      &#1089;&#1090;&#1088;&#1086;&#1082;&#1080; (command line) &#1084;&#1086;&#1075;&#1091;&#1090; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1086;&#1074;&#1072;&#1090;&#1100; &#1090;&#1086;&#1090; &#1078;&#1077; &#1082;&#1086;&#1076;, &#1082;&#1086;&#1090;&#1086;&#1088;&#1099;&#1081; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1091;&#1077;&#1090;&#1089;&#1103;
      &#1087;&#1088;&#1080; &#1088;&#1072;&#1079;&#1088;&#1072;&#1073;&#1086;&#1090;&#1082;&#1072;&#1093; web-&#1087;&#1088;&#1080;&#1083;&#1086;&#1078;&#1077;&#1085;&#1080;&#1081;.
       &#1069;&#1090;&#1086; &#1076;&#1086;&#1089;&#1090;&#1080;&#1075;&#1072;&#1077;&#1090;&#1089;&#1103; &#1087;&#1088;&#1080;&#1084;&#1077;&#1085;&#1077;&#1085;&#1080;&#1077;&#1084; &#1103;&#1079;&#1099;&#1082;&#1072; TCL - &#1074;&#1085;&#1077; &#1080; &#1074;&#1085;&#1091;&#1090;&#1088;&#1080; Apache &#1089;&#1077;&#1088;&#1074;&#1077;&#1088;&#1072;.
    </p><p style="width:90%">
      &#1042; &#1101;&#1090;&#1086;&#1084; &#1076;&#1086;&#1082;&#1091;&#1084;&#1077;&#1085;&#1090;&#1077; &#1084;&#1099; &#1087;&#1086;&#1089;&#1090;&#1072;&#1088;&#1072;&#1077;&#1084;&#1089;&#1103; &#1087;&#1086;&#1084;&#1086;&#1095;&#1100; &#1074;&#1072;&#1084; &#1073;&#1099;&#1089;&#1090;&#1088;&#1086; &#1088;&#1072;&#1079;&#1086;&#1073;&#1088;&#1072;&#1090;&#1100;&#1089;&#1103; &#1082;&#1072;&#1082;
      &#1084;&#1086;&#1078;&#1085;&#1086; &#1089;&#1086;&#1079;&#1076;&#1072;&#1074;&#1072;&#1090;&#1100; web- &#1087;&#1088;&#1080;&#1083;&#1086;&#1078;&#1077;&#1085;&#1080;&#1103;, &#1080; &#1087;&#1086;&#1082;&#1072;&#1079;&#1072;&#1090;&#1100; &#1074;&#1072;&#1084; &#1076;&#1072;&#1083;&#1100;&#1085;&#1077;&#1081;&#1096;&#1080;&#1077; 
      &#1087;&#1091;&#1090;&#1080; &#1076;&#1083;&#1103; &#1088;&#1072;&#1079;&#1088;&#1072;&#1073;&#1086;&#1090;&#1082;&#1080; &#1087;&#1088;&#1080;&#1083;&#1086;&#1078;&#1077;&#1085;&#1080;&#1081; &#1080;&#1089;&#1087;&#1086;&#1083;&#1100;&#1079;&#1091;&#1103; rivet &#1076;&#1083;&#1103; &#1089;&#1072;&#1084;&#1099;&#1093;
      &#1088;&#1072;&#1079;&#1085;&#1086;&#1086;&#1073;&#1088;&#1072;&#1079;&#1085;&#1099;&#1093; &#1087;&#1088;&#1080;&#1084;&#1077;&#1085;&#1077;&#1085;&#1080;&#1081;.
    </p><p style="width:90%">
      &#1069;&#1090;&#1072; &#1076;&#1086;&#1082;&#1091;&#1084;&#1077;&#1085;&#1090;&#1072;&#1094;&#1080;&#1103; - &#1082;&#1072;&#1082; &#1080; &#1074;&#1077;&#1089;&#1100; rivet, &#1080; &#1090;&#1077;&#1084; &#1073;&#1086;&#1083;&#1077;&#1077; &#1077;&#1077; &#1087;&#1077;&#1088;&#1077;&#1074;&#1086;&#1076; &#1085;&#1072;
      &#1088;&#1091;&#1089;&#1089;&#1082;&#1080;&#1081; - &#1101;&#1090;&#1086; &#1088;&#1072;&#1073;&#1086;&#1090;&#1072;, &#1082;&#1086;&#1090;&#1086;&#1088;&#1072;&#1103; &#1085;&#1077; &#1079;&#1072;&#1074;&#1088;&#1077;&#1096;&#1077;&#1085;&#1072;, &#1080; &#1073;&#1091;&#1076;&#1077;&#1084; &#1085;&#1072;&#1076;&#1077;&#1103;&#1090;&#1100;&#1089;&#1103;
      &#1073;&#1091;&#1076;&#1077;&#1090; &#1087;&#1088;&#1086;&#1076;&#1086;&#1083;&#1078;&#1072;&#1090;&#1100;&#1089;&#1103; &#1077;&#1097;&#1077; &#1086;&#1095;&#1077;&#1085;&#1100; &#1076;&#1086;&#1083;&#1075;&#1086;.
      &#1045;&#1089;&#1083;&#1080; &#1074;&#1099; &#1074;&#1080;&#1076;&#1080;&#1090;&#1077; &#1095;&#1090;&#1086; &#1095;&#1090;&#1086;-&#1090;&#1086; &#1075;&#1076;&#1077;-&#1090;&#1086; &#1087;&#1086;&#1095;&#1077;&#1084;&#1091;-&#1090;&#1086; &#1085;&#1077; &#1090;&#1072;&#1082; - &#1090;&#1086; &#1087;&#1080;&#1096;&#1080;&#1090;&#1077;
      &#1086;&#1073; &#1101;&#1090;&#1086;&#1084;, &#1084;&#1099; &#1089;&#1072;&#1084;&#1080; &#1079;&#1072; &#1074;&#1089;&#1077;&#1084; &#1085;&#1077; &#1091;&#1075;&#1083;&#1103;&#1076;&#1080;&#1084; &#1073;&#1077;&#1079; &#1074;&#1072;&#1096;&#1077;&#1081; &#1087;&#1086;&#1084;&#1086;&#1097;&#1080;.
    </p></div></div><div class="navfooter"><hr><table width="100%" summary="Navigation footer"><tr><td width="40%" align="left">�</td><td width="20%" align="center">�</td><td width="40%" align="right">�<a accesskey="n" href="installation.html.ru"><img src="images/next.png" alt="Next"></a></td></tr><tr><td width="40%" align="left" valign="top">�</td><td width="20%" align="center">�</td><td width="40%" align="right" valign="top">�&#1059;&#1089;&#1090;&#1072;&#1085;&#1086;&#1074;&#1082;&#1072; Apache Rivet</td></tr></table></div></body></html>
