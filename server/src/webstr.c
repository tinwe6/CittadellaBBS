/*
 *  Copyright (C) 2003-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                      (C) M. Caldarelli *
*                                                                             *
* File: webstr.c                                                              *
*       templates per la costruzione delle pagine del cittaweb.               *
******************************************************************************/

#include "config.h"

#ifdef USE_CITTAWEB

#include "webstr.h"

#include "cittaserver.h"

/* Strutture per la gestione delle lingue. */
const char * http_lang[] =
	{"it", "en", "fr", "de"};

const char * http_lang_name[] =
	{"Italiano", "English", "Fran&ccedil;ais", "Deutsch"};


const char HTML_HEADER[] =
/*
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\r\n";
*/
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n<HTML>\r\n";


const char HTML_404[] =
"<HTML><HEAD>\r\n"
"<TITLE>404 Not Found</TITLE>\r\n"
"</HEAD><BODY>\r\n"
"<H1>Not Found</H1>\r\n"
"The requested URL was not found on this server.<P>\r\n"
"</BODY></HTML>\r\n";

const char HTML_NOT_AVAIL[] =
"<HTML><HEAD>\r\n"
"<TITLE>403 Forbidden</TITLE>\r\n"
"</HEAD><BODY>\r\n"
"<H1>Not Found</H1>\r\n"
"The requested room is not available from the web. "
"Please login in the BBS.<P>\r\n"
"</BODY></HTML>\r\n";

const char HTML_START_CSS[] =
"<STYLE TYPE=\"text/css\">\r\n"
        "<!--\r\n";

const char HTML_END_CSS[] =
"-->\r\n"
"</STYLE>\r\n";

const char HTML_USERLIST_CSS[] =
"BODY { background: Black; color: Silver; font-family: monospace}\r\n"
"P.titolo { color: Red; font-family: \"Lucida Grande\", sans-serif;"
                 "font-weight: normal; font-size: 16px}\r\n"
"P.userlist { color: Silver; font-family: monospace}\r\n"
"IMG { border: 0px }\r\n"
"SPAN.username { color: white }\r\n"
"SPAN.level { color: red; font-weight: bold }\r\n"
"SPAN.data { color: fuchsia }\r\n"
"SPAN.chiamate { color: Blue; font-weight: bold }\r\n"
"SPAN.post { color: aqua; font-weight: bold }\r\n"
"SPAN.mail { color: white; font-weight: bold }\r\n"
"SPAN.xmsg { color: yellow; font-weight: bold }\r\n"
"SPAN.chat { color: green; font-weight: bold }\r\n"
        //"      SPAN.link { color: Blue; font: 14px \"Lucida Grande\"; font-style }\r\n"
"A:link { color: red; font-style: italic }\r\n"
"A:visited { color: maroon }\r\n"
"A:active { color: fuchsia }\r\n"
"H1    { color: green }\r\n"
"H4    { color: green }\r\n"
"H5    { color: green; font-style: oblique; text-align: right}\r\n"
"B     { font-weight: 700 }\r\n"
"I     { font-style: oblique }\r\n"
"FGY   { color: yellow }\r\n"
"FGB   { color: blue }\r\n"
"FGR   { color: red }\r\n"
"BGY   { background-color: yellow}\r\n"
"BGR   { background-color: red}\r\n"
"GK    { color: green; background-color: black}\r\n"
"MK    { color: fuchsia; background-color: black}\r\n";

const char HTML_BLOGLIST_CSS[] =
"BODY { background: Black; color: Silver; font-family: monospace}\r\n"
"P.titolo { color: Red; font: 16px \"Lucida Grande\";"
                 "font-weight: normal}\r\n"
"P.blogs { color: Silver; font-family: monospace}\r\n"
"IMG { border: 0px }\r\n"
"A:link         { color: yellow; text-decoration: none}\r\n"
"A:visited      { color: yellow; text-decoration: none}\r\n"
"A:active       { color: fuchsia }\r\n"
"SPAN.user { color: White }\r\n"
"SPAN.numpost { color: Blue }\r\n"
"SPAN.level { color: Red; font-weight: bold }\r\n"
"SPAN.chiamate { color: Blue; font-weight: bold }\r\n"
"SPAN.post { color: aqua; font-weight: bold }\r\n"
"SPAN.mail { color: White; font-weight: bold }\r\n"
"SPAN.xmsg { color: Yellow; font-weight: bold }\r\n"
"SPAN.chat { color: Green; font-weight: bold }\r\n"
"H1        { color: green }\r\n"
"H4        { color: green }\r\n"
"H5        { color: green; font-style: oblique; text-align: right}\r\n"
"B         { font-weight: 700 }\r\n"
"I { font-style: oblique }\r\n"
"SPAN.LastPost    { font: 10px \"Lucida Grande\";"
                        " font-weight: normal;"
                        " font-style: Italic; color: white }\r\n"
"FGY   { color: yellow }\r\n"
"FGB   { color: blue }\r\n"
"FGR   { color: red }\r\n"
"BGY   { background-color: yellow}\r\n"
"BGR   { background-color: red}\r\n"
"GK    { color: green; background-color: black}\r\n"
"MK    { color: fuchsia; background-color: black}\r\n";

const char HTML_INDEX_CSS[] =
"BODY { background: Black; color: Silver; font-family: monospace}\r\n"
"#lastpost   { background: black; align: center; text-align: center; border: thin dashed silver; padding: 12px 12px 12px 12px; width: 80%%; position:relative; left:10%%; right:10%%}\r\n"
        /*
"#header { position: fixed; width: 100%%; height: auto; top: 0; right: 0; bottom: auto; left: 0}\r\n"
"#side { position: fixed; width: 40em; height: auto; top: 15%%; right: auto; bottom: 100px; left: 0}\r\n"
"#main { position: fixed; width: auto; height: auto; top: 15%%; right: 0; bottom: 100px; left: 10em}\r\n"
"#footer { position: fixed; width: 80%%; height: auto; top: auto; right: 0; bottom: 0; left: 0}\r\n"
        */
"A:link         { color: aqua; text-decoration: none}\r\n"
"A:visited      { color: aqua; text-decoration: none}\r\n"
"A:active       { color: fuchsia }\r\n"
"IMG            { border: 0px }\r\n"
"TABLE.center   { width: 76%%; margin-left: 12%%; margin-right: 12%%; text-align: center }\r\n"
"TABLE.rooms    { width: 84%%; margin-left: 8%%; margin-right: 8%%; text-align: left }\r\n"
"DIV.center     { text-align: center }\r\n"
"H1             { color: green }\r\n"
"H4             { color: green }\r\n"
"H5             { color: green; font-style: oblique; text-align: right}\r\n"
"B              { font-weight: 700 }\r\n"
"I              { font-style: oblique }\r\n"
"SPAN.lastmsg   {Color: white}\r\n"
"SPAN.bbs       { color: yellow; font-weight: 700 }\r\n"
"SPAN.postlink  { color: teal; font-weight: 700 font-style: oblique }\r\n"
"SPAN.msgh      { color: lime }\r\n"
"SPAN.notavail  { color: red; font-style: oblique }\r\n"
"SPAN.FGY       { color: yellow }\r\n"
"SPAN.FGR       { color: red }\r\n"
"SPAN.BGY       { background-color: yellow}\r\n"
"SPAN.BGY       { background-color: red}\r\n"
"SPAN.GK        { color: green; background-color: black}\r\n"
"SPAN.MK        { color: fuchsia; background-color: black}\r\n";

const char HTML_ROOM_CSS[] =
" BODY { background: Black; color: Silver; font-family: monospace}\r\n"
" A:link         { color: aqua; text-decoration: none}\r\n"
" A:visited      { color: aqua; text-decoration: none}\r\n"
" A:active       { color: fuchsia }\r\n"
"      IMG { border: 0px }\r\n"
" TABLE.center   { width: 76%%; margin-left: 12%%; margin-right: 12%%; text-align: center }\r\n"
" TABLE.rooms    { width: 84%%; margin-left: 8%%; margin-right: 8%%; text-align: left }\r\n"
" DIV.center     { text-align: center }\r\n"
" H1             { color: green }\r\n"
" H4             { color: green }\r\n"
" H5             { color: green; font-style: oblique; text-align: right}\r\n"
" B              { font-weight: 700 }\r\n"
" I              { font-style: oblique }\r\n"
" SPAN.bbs       { color: yellow; font-weight: 700 }\r\n"
" SPAN.info      { color: lime }\r\n"
" SPAN.roomdesc  { color: aqua }\r\n"
" SPAN.roomdesctit { color: aqua; font-weight:800 }\r\n"
" SPAN.msghdr    { color: lime }\r\n"
" SPAN.subj      { color: yellow }\r\n"
" SPAN.msgbdy    { color: silver }\r\n"
" SPAN.FGR       { color: red }\r\n"
" SPAN.room      { color: yellow; text-decoration: underline }\r\n"
" SPAN.postnum   { color: fuchsia; text-decoration: underline }\r\n"
" SPAN.link      { color: aqua; text-decoration: underline }\r\n"
" SPAN.file      { color: green; text-decoration: underline }\r\n";

const char HTML_META[] =
"<META http-equiv=Content-Type content=\"text/html; charset=iso-8859-1\">\r\n"
"<META content=\"MSHTML 5.50.4926.2500\" name=GENERATOR>\r\n"
"<META content=\"Cittadella BBS\" name=Author>\r\n"
"<META content=\"Cittadella BBS Preview\" name=Description></HEAD>\r\n";

const char HTML_TAIL[] =
"<TABLE class=\"center\" width=\"95%%\" border\"0\"><COLGROUP width=\"100%%\"><COL width=\"auto\"><COL width\"150px\"></COLGROUP>"
"<TR><TD align=\"left\"><A href=\"/index\">Torna all'indice delle room</A></TD>"
 "<TD><A href=\"telnet://%s:%d/\"><IMG SRC=\"/images/join-small.png\" alt=\"Join\"></A></TD></TR>"
"<TR><TD>&nbsp;</TD><TD>&nbsp;</TD></TR>"
"<TR><TD align=\"left\"><a href=\"http://validator.w3.org/check?uri=referer\"><img src=\"http://www.w3.org/Icons/valid-html401\" alt=\"Valid HTML 4.01!\" height=\"31\" width=\"88\"></a>\r\n"
"<a href=\"http://jigsaw.w3.org/css-validator/\"><img style=\"border:0;width:88px;height:31px\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\"></a></TD><TD>&nbsp;</TD></TR>\r\n"
"</TABLE>\r\n"
"</BODY></HTML>\r\n";

const char HTML_TAILINDEX[] =
"<TABLE class=\"center\" width=\"90%%\" border\"0\"><COLGROUP width=\"100%%\"><COL width=\"auto\"><COL width\"150px\"></COLGROUP>"
"<TR><TD align=\"left\"></TD>\r\n"
 "<TD><A href=\"telnet://%s:%d/\"><IMG SRC=\"/images/join-small.png\" alt=\"Join\"></A></TD></TR>"
"<TR><TD>&nbsp;</TD><TD>&nbsp;</TD></TR>"
"<TR><TD align=\"left\"><a href=\"http://validator.w3.org/check?uri=referer\"><img src=\"http://www.w3.org/Icons/valid-html401\" alt=\"Valid HTML 4.01!\" height=\"31\" width=\"88\"></a>\r\n"
"<a href=\"http://jigsaw.w3.org/css-validator/\"><img style=\"border:0;width:88px;height:31px\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\"></a></TD><TD>&nbsp;</TD></TR>\r\n"
"</TABLE>\r\n"
"</BODY></HTML>\r\n";

const char HTML_INDEX_BODY[] =
"<BODY>\r\n"
"<DIV id=\"header\"><H1>%s</H1>\r\n"
" <TABLE class=\"center\" border=\"1\" cellspacing=\"0\">\r\n"
"  <TR><TD>\r\n"
"   <DIV>%s</DIV>\r\n"
"  </TD></TR>\r\n"
" </TABLE>\r\n";

const char * HTML_INDEX_TITLE[HTTP_LANG_NUM];
const char HTML_INDEX_TITLE_IT[] = "Cittadella BBS Anteprima Web";
const char HTML_INDEX_TITLE_EN[] = "Cittadella BBS Web Preview";
const char HTML_INDEX_TITLE_FR[] = "Cittadella BBS Avant-Premi&egrave;re Web";
const char HTML_INDEX_TITLE_DE[] = "Cittadella BBS Netz-Vorbetrachtung";

const char * HTML_INDEX_PREAMBLE[HTTP_LANG_NUM];
const char HTML_INDEX_PREAMBLE_IT[] =
"Questa e le pagine successive, sono un'anteprima di quello che sta "
"accadendo<SPAN class=\"FGR\"> ora </SPAN>in "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN> . "
"Questa &egrave; solo la lista delle room accessibili via web, ma ce ne sono "
"molte altre all'interno. Se vuoi sapere cos'&egrave; "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN> vai a: "
"<B>http://bbs.cittadellabbs.it</B>. "
"Se non riesci ad aspettare e vuoi vedere subito "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN> clicca "
"<A href=\"telnet://bbs.cittadelabbs.it:4001\">qui</A> e scrivi "
"<I><B>Ospite</B></I> al prompt <I><B>Nome:</B></I> . <BR> "
"E buon divertimento!";

const char HTML_INDEX_PREAMBLE_EN[] =
"This and the other pages are just a preview of what's happening in "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN> right "
"<SPAN class=\"FGR\">now</SPAN>. "
"This is a list of the rooms that are published on the web, you can find many "
"more of them inside. If you want to know what <SPAN class=\"bbs\">Cittadella "
"BBS</SPAN> is (and you know a little Italian) go to "
"<B>http://bbs.cittadellabbs.it</B>. "
"If you can't wait and want to try <SPAN class=\"bbs\">Cittadella BBS</SPAN> "
"now click <A href=\"telnet://bbs.cittadelabbs.it:4001\">here</A> and write "
"<I><B>Guest</B></I> at the <I><B>Nome:</B></I> prompt. <BR> "
"Have Fun!";

const char HTML_INDEX_PREAMBLE_FR[] =
"Cette page et les suivantes sont un aper&ccedil;u de ce qui se "
"passe<SPAN class=\"FGR\"> maintenant </SPAN>"
"dans <SPAN class=\"bbs\">Cittadella BBS</SPAN> . "
"Celle-ci est simplement la liste des room accessibles du web, mais il y en a "
"beaucoup d'autres &agrave; l'interieur. Si tu veux savoir ce que c'est "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN> va &agrave;: "
"<B>http://bbs.cittadellabbs.it</B>. Si tu ne veux pas attendre et tu veux "
"voir tout de suite <SPAN class=\"bbs\">Cittadella BBS</SPAN>, clique "
"<A href=\"telnet://bbs.cittadelabbs.it:4001\">qui</A> et &eacute;cris "
"<I><B>Guest</B></I> au prompt <i><b>Nome:</B></I> . <BR> "
"Et bon divertissement!";

const char HTML_INDEX_PREAMBLE_DE[] =
"Dieses und die anderen Seiten sind eine Vorbetrachtung gerade von, "
"was in <SPAN class=\"bbs\"> Cittadella BBS </SPAN> im Augenblick "
"<span class=\"FGR\"> geschieht </span>.  Dieses ist eine Liste der "
"R&#228;ume, die auf dem Netz ver&#246;ffentlicht werden, Sie kann viel mehr "
"von ihnen nach innen finden.  Wenn Sie wissen m&#246;chten, was "
"<span class=\"bbs\"> Cittadella BBS </span>  (und Sie, italienisches wenig "
"wissen Sie), gehen zu <B>http://bbs.cittadellabbs.it</B> ist. "
"Wenn Sie nicht warten k&#246;nnen und "
"<SPAN class=\"bbs\"> Cittadella BBS </SPAN> versuchen zu w&#252;nschen "
"klicken Sie jetzt <a href=\"telnet://bbs.cittadelabbs.it:4001\" > hier </a> "
"und schreiben Sie <i><b> Guest </b></i>  beim <i><b> Nome:  </b></i> "
"Aufforderung.";

const char * HTML_INDEX_ROOM_LIST[HTTP_LANG_NUM];
const char HTML_INDEX_ROOM_LIST_IT[] = "Lista Room disponibili sul Web";
const char HTML_INDEX_ROOM_LIST_EN[] = "List of Rooms available on the Web";
const char HTML_INDEX_ROOM_LIST_FR[] = "Liste Room disponibiles sur le Web";
const char HTML_INDEX_ROOM_LIST_DE[] = "Liste der Zimmer frei auf dem Netz";

const char * HTML_INDEX_USERLIST[HTTP_LANG_NUM];
const char HTML_INDEX_USERLIST_IT[] = "Lista Utenti BBS";
const char HTML_INDEX_USERLIST_EN[] = "BBS Users List";
const char HTML_INDEX_USERLIST_FR[] = "Liste Utilisateurs BBS";
const char HTML_INDEX_USERLIST_DE[] = "BBS Benutzer-Liste";

const char * HTML_INDEX_BLOGLIST[HTTP_LANG_NUM];
const char HTML_INDEX_BLOGLIST_IT[] = "I Blog degli utenti della BBS";
const char HTML_INDEX_BLOGLIST_EN[] = "BBS Users' Blogs";
const char HTML_INDEX_BLOGLIST_FR[] = "Liste des Blogs des utilisateurs BBS";
const char HTML_INDEX_BLOGLIST_DE[] = "BBS Benutzer-Blogs";

const char * HTML_INDEX_LASTMSG[HTTP_LANG_NUM];
const char HTML_INDEX_LASTMSG_IT[] =
"Ultimo messaggio inserito in <B>Cittadella BBS</B>:";
const char HTML_INDEX_LASTMSG_EN[] =
"Last message inserted in <B>Cittadella BBS</B>:";
const char HTML_INDEX_LASTMSG_FR[] =
"Dernier message ins&eacute;r&eacute; dans <B>Cittadella BBS</B>:";
const char HTML_INDEX_LASTMSG_DE[] =
"Letzte Anzeige eingesetzt in <B>Cittadella BBS</B>:";

const char * HTML_INDEX_LASTMAIL[HTTP_LANG_NUM];
const char HTML_INDEX_LASTMAIL_IT[] =
"L'ultimo messaggio inserito in <B>Cittadella BBS</B> &egrave; un <B>mail privato</B>";
const char HTML_INDEX_LASTMAIL_EN[] =
"The last message inserted in <B>Cittadella BBS</B> is a <B>private mail</B>";
const char HTML_INDEX_LASTMAIL_FR[] =
"Le dernier message ins&eacute;r&eacute; dans <B>Cittadella BBS</B> est un mail priv&eacute;";
const char HTML_INDEX_LASTMAIL_DE[] =
"Das letzte Anzeige eingesetzt in <B>Cittadella BBS</B> ist einen <B>privat Mail</B>";
const char * HTML_INDEX_READIT[HTTP_LANG_NUM];
const char HTML_INDEX_READIT_IT[] =
        "Leggi ora il messaggio!";
const char HTML_INDEX_READIT_EN[] =
        "Read now the message!";
const char HTML_INDEX_READIT_FR[] =
        "Lis maintenant le message!";
const char HTML_INDEX_READIT_DE[] =
        "Lies jetzt das Anzeige!";
const char * HTML_INDEX_NOTAVAIL[HTTP_LANG_NUM];
const char HTML_INDEX_NOTAVAIL_IT[] =
"Questo messaggio <STRONG>non</STRONG> &egrave; disponibile dal CittaWeb.\r\n"
"Per leggerlo, collegati <STRONG>ora</STRONG> a <STRONG>Cittadella BBS</STRONG>!";
const char HTML_INDEX_NOTAVAIL_EN[] =
"This message is <STRONG>not</STRONG> available from CittaWeb.\r\n"
"To read it, join us <STRONG>now</STRONG> in <STRONG>Cittadella BBS</STRONG>!";
const char HTML_INDEX_NOTAVAIL_FR[] =
"Ce message <STRONG>n'est pas</STRONG> disponible du CittaWeb.\r\n"
"Pour le lire, rejoins-nous <STRONG>maintenant</STRONG> dans <STRONG>Cittadella BBS</STRONG>!";
const char HTML_INDEX_NOTAVAIL_DE[] =
"This message is <STRONG>not</STRONG> available from CittaWeb.\r\n"
"To read it, join us <STRONG>now</STRONG> in <STRONG>Cittadella BBS</STRONG>!";

const char * HTML_INDEX_NONEW[HTTP_LANG_NUM];
const char HTML_INDEX_NONEW_IT[] =
        "Non sono stati lasciati nuovi messaggi dall'ultimo reboot del server.";
const char HTML_INDEX_NONEW_EN[] =
        "No new messages have been posted since the last reboot of the server.";
const char HTML_INDEX_NONEW_FR[] =
        "Aucun nouveau message n'a &eacute;t&eacute; laiss&eacute; depuis le dernier reboot.";
const char HTML_INDEX_NONEW_DE[] =
        "No new messages have been posted since the last reboot of the server.";

const char * HTML_ROOM_PREAMBLE[HTTP_LANG_NUM];
const char HTML_ROOM_PREAMBLE_IT[] =
"In questa pagina puoi leggere gli ultimi messaggi e seguire "
"quello che sta accadendo<SPAN class=\"FGR\"> ora </SPAN>nella "
"room di <SPAN class=\"bbs\">Cittadella BBS</SPAN> da te prescelta. "
"Se vuoi accedere a tutti i messaggi o scriverne personalmente, "
"collegati subito a <SPAN class=\"bbs\">Cittadella BBS</SPAN>!";

const char HTML_ROOM_PREAMBLE_EN[] =
"In this page you can read the last messages and follow what's happening "
"right <SPAN class=\"FGR\">now</SPAN> in the selected room. "
"If you want to access all messages or want to post some, "
"connect immediatly to <SPAN class=\"bbs\">Cittadella BBS</SPAN>!";

const char HTML_ROOM_PREAMBLE_FR[] =
"Dans cette page tu peux lire les derniers messages e suivre ce qui se "
"passe<SPAN class=\"FGR\"> maintenant </SPAN>dans la room que tu as choisie. "
"Pour acceder &agrave; tous le messages ou pour en laisser personnellement, "
"connectes-toi immediatement &agrave; "
"<SPAN class=\"bbs\">Cittadella BBS</SPAN>!";

const char HTML_ROOM_PREAMBLE_DE[] =
"In dieser Seite k&#246;nnen Sie die letzten Anzeigen lesen und folgen, was "
"im <span class=\"FGR\"> Augenblick </span>  im vorgew&#228;hlten Raum "
"geschieht. Wenn Sie alle Anzeigen zug&#228;nglich machen oder einiges "
"bekanntgeben w&#252;nschen m&#246;chten, schlie&#223;en Sie immediatly an "
"<span class=\"bbs\"> Cittadella BBS </span> an!";

/* Prototipi funzioni in questo file: */

void webstr_init(void);

/****************************************************************************/

/* Inizializza i puntatori alle stringhe */
void webstr_init(void)
{
	HTML_INDEX_TITLE[0] = HTML_INDEX_TITLE_IT;
	HTML_INDEX_TITLE[1] = HTML_INDEX_TITLE_EN;
	HTML_INDEX_TITLE[2] = HTML_INDEX_TITLE_FR;
	HTML_INDEX_TITLE[3] = HTML_INDEX_TITLE_DE;

	HTML_INDEX_PREAMBLE[0] = HTML_INDEX_PREAMBLE_IT;
	HTML_INDEX_PREAMBLE[1] = HTML_INDEX_PREAMBLE_EN;
	HTML_INDEX_PREAMBLE[2] = HTML_INDEX_PREAMBLE_FR;
	HTML_INDEX_PREAMBLE[3] = HTML_INDEX_PREAMBLE_DE;

	HTML_INDEX_ROOM_LIST[0] = HTML_INDEX_ROOM_LIST_IT;
	HTML_INDEX_ROOM_LIST[1] = HTML_INDEX_ROOM_LIST_EN;
	HTML_INDEX_ROOM_LIST[2] = HTML_INDEX_ROOM_LIST_FR;
	HTML_INDEX_ROOM_LIST[3] = HTML_INDEX_ROOM_LIST_DE;

	HTML_INDEX_USERLIST[0] = HTML_INDEX_USERLIST_IT;
	HTML_INDEX_USERLIST[1] = HTML_INDEX_USERLIST_EN;
	HTML_INDEX_USERLIST[2] = HTML_INDEX_USERLIST_FR;
	HTML_INDEX_USERLIST[3] = HTML_INDEX_USERLIST_DE;

	HTML_INDEX_BLOGLIST[0] = HTML_INDEX_BLOGLIST_IT;
	HTML_INDEX_BLOGLIST[1] = HTML_INDEX_BLOGLIST_EN;
	HTML_INDEX_BLOGLIST[2] = HTML_INDEX_BLOGLIST_FR;
	HTML_INDEX_BLOGLIST[3] = HTML_INDEX_BLOGLIST_DE;

	HTML_INDEX_LASTMSG[0] = HTML_INDEX_LASTMSG_IT;
	HTML_INDEX_LASTMSG[1] = HTML_INDEX_LASTMSG_EN;
	HTML_INDEX_LASTMSG[2] = HTML_INDEX_LASTMSG_FR;
	HTML_INDEX_LASTMSG[3] = HTML_INDEX_LASTMSG_DE;

	HTML_INDEX_LASTMAIL[0] = HTML_INDEX_LASTMAIL_IT;
	HTML_INDEX_LASTMAIL[1] = HTML_INDEX_LASTMAIL_EN;
	HTML_INDEX_LASTMAIL[2] = HTML_INDEX_LASTMAIL_FR;
	HTML_INDEX_LASTMAIL[3] = HTML_INDEX_LASTMAIL_DE;

	HTML_INDEX_READIT[0] = HTML_INDEX_READIT_IT;
	HTML_INDEX_READIT[1] = HTML_INDEX_READIT_EN;
	HTML_INDEX_READIT[2] = HTML_INDEX_READIT_FR;
	HTML_INDEX_READIT[3] = HTML_INDEX_READIT_DE;

	HTML_INDEX_NOTAVAIL[0] = HTML_INDEX_NOTAVAIL_IT;
	HTML_INDEX_NOTAVAIL[1] = HTML_INDEX_NOTAVAIL_EN;
	HTML_INDEX_NOTAVAIL[2] = HTML_INDEX_NOTAVAIL_FR;
	HTML_INDEX_NOTAVAIL[3] = HTML_INDEX_NOTAVAIL_DE;

	HTML_INDEX_NONEW[0] = HTML_INDEX_NONEW_IT;
	HTML_INDEX_NONEW[1] = HTML_INDEX_NONEW_EN;
	HTML_INDEX_NONEW[2] = HTML_INDEX_NONEW_FR;
	HTML_INDEX_NONEW[3] = HTML_INDEX_NONEW_DE;

	HTML_ROOM_PREAMBLE[0] = HTML_ROOM_PREAMBLE_IT;
	HTML_ROOM_PREAMBLE[1] = HTML_ROOM_PREAMBLE_EN;
	HTML_ROOM_PREAMBLE[2] = HTML_ROOM_PREAMBLE_FR;
	HTML_ROOM_PREAMBLE[3] = HTML_ROOM_PREAMBLE_DE;

}

#endif /* USE_CITTAWEB */
