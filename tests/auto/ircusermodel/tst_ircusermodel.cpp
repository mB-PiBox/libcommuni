/*
 * Copyright (C) 2008-2013 The Communi Project
 *
 * This test is free, and not covered by the LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircusermodel.h"
#include "ircconnection.h"
#include "ircbuffermodel.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "irc.h"
#include <QtTest/QtTest>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

static bool caseInsensitiveLessThan(const QString& s1, const QString& s2)
{
    return s1.compare(s2, Qt::CaseInsensitive) < 0;
}

static bool caseInsensitiveGreaterThan(const QString& s1, const QString& s2)
{
    return s1.compare(s2, Qt::CaseInsensitive) > 0;
}

class tst_IrcUserModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaults();
    void testSorting();
    void testActivity();

private:
    void waitForWritten(const QByteArray& data = QByteArray());

    QPointer<QTcpServer> server;
    QPointer<QTcpSocket> serverSocket;
    QPointer<IrcConnection> connection;
    QPointer<QAbstractSocket> clientSocket;
};

void tst_IrcUserModel::init()
{
    server = new QTcpServer(this);
    QVERIFY(server->listen());

    connection = new IrcConnection(this);
    connection->setUserName("user");
    connection->setNickName("nick");
    connection->setRealName("real");
    connection->setHost(server->serverAddress().toString());
    connection->setPort(server->serverPort());

    connection->open();
    if (!server->waitForNewConnection(200))
        QEXPECT_FAIL("", "The address is not available", Abort);
    serverSocket = server->nextPendingConnection();
    QVERIFY(serverSocket);

    clientSocket = connection->socket();
    QVERIFY(clientSocket);
    QVERIFY(clientSocket->waitForConnected());
}

void tst_IrcUserModel::cleanup()
{
    delete server;
    delete connection;
}

void tst_IrcUserModel::waitForWritten(const QByteArray& data)
{
    if (!data.isNull())
        serverSocket->write(data);
    QVERIFY(serverSocket->waitForBytesWritten());
    QVERIFY(clientSocket->waitForReadyRead());
}

void tst_IrcUserModel::testDefaults()
{
    IrcUserModel model;
    QCOMPARE(model.count(), 0);
    QVERIFY(model.names().isEmpty());
    QVERIFY(model.users().isEmpty());
    QCOMPARE(model.displayRole(), Irc::TitleRole);
    QVERIFY(!model.channel());
    QVERIFY(!model.dynamicSort());
}

const char* freenode_welcome =
        ":moorcock.freenode.net NOTICE * :*** Looking up your hostname...\r\n"
        ":moorcock.freenode.net NOTICE * :*** Checking Ident\r\n"
        ":moorcock.freenode.net NOTICE * :*** Found your hostname\r\n"
        ":moorcock.freenode.net NOTICE * :*** No Ident response\r\n"
        ":moorcock.freenode.net 001 communi :Welcome to the freenode Internet Relay Chat Network communi\r\n"
        ":moorcock.freenode.net 002 communi :Your host is moorcock.freenode.net[50.22.136.18/6667], running version ircd-seven-1.1.3\r\n"
        ":moorcock.freenode.net 003 communi :This server was created Mon Dec 31 2012 at 15:37:06 CST\r\n"
        ":moorcock.freenode.net 004 communi moorcock.freenode.net ircd-seven-1.1.3 DOQRSZaghilopswz CFILMPQSbcefgijklmnopqrstvz bkloveqjfI\r\n"
        ":moorcock.freenode.net 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=freenode KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server\r\n"
        ":moorcock.freenode.net 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server\r\n"
        ":moorcock.freenode.net 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server\r\n"
        ":moorcock.freenode.net 251 communi :There are 231 users and 88216 invisible on 29 servers\r\n"
        ":moorcock.freenode.net 252 communi 36 :IRC Operators online\r\n"
        ":moorcock.freenode.net 253 communi 12 :unknown connection(s)\r\n"
        ":moorcock.freenode.net 254 communi 49792 :channels formed\r\n"
        ":moorcock.freenode.net 255 communi :I have 4723 clients and 1 servers\r\n"
        ":moorcock.freenode.net 265 communi 4723 7446 :Current local users 4723, max 7446\r\n"
        ":moorcock.freenode.net 266 communi 88447 92550 :Current global users 88447, max 92550\r\n"
        ":moorcock.freenode.net 250 communi :Highest connection count: 7447 (7446 clients) (1286042 connections received)\r\n"
        ":moorcock.freenode.net 375 communi :- moorcock.freenode.net Message of the Day -\r\n"
        ":moorcock.freenode.net 372 communi :- Welcome to moorcock.freenode.net in ...\r\n"
        ":moorcock.freenode.net 376 communi :End of /MOTD command.\r\n";

const char* freenode_join =
        ":communi!~communi@hidd.en JOIN #freenode\r\n"
        ":moorcock.freenode.net 332 communi #freenode :Welcome to #freenode | Staff are voiced; some may also be on /stats p -- feel free to /msg us at any time | FAQ: http://freenode.net/faq.shtml | Unwelcome queries? Use /mode your_nick +R to block them. | Channel guidelines: http://freenode.net/poundfreenode.shtml | Blog: http://blog.freenode.net | Please don't comment on spam/trolls.\r\n"
        ":moorcock.freenode.net 333 communi #freenode erry 1379357591\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :communi straterra absk007 pefn xlys Gromit TooCool Sambler gat0rs KarneAsada danis_963 Kiryx chrismeller deefloo black_male sxlnxdx bjork Kinny phobos_anomaly T13|sleeps JuxTApose Kolega2357 rorx techhelper1 hermatize Azimi iqualfragile fwilson skasturi mwallacesd mayday Guest76549 mcjohansen MangaKaDenza ARISTIDES ketas `- claptor ylluminate Cooky Brand3n cheater_1 Kirito digitaloktay Will| Iarfen abrotman smurfy Inaunt +mist Karol RougeR_\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :publickeating An_Ony_Moose michagogo Guest915` davidfg4 Ragnor s1lent_1 keee GingerGeek[Away] hibari derp S_T_A_N anonymuse asantoni road|runner LLckfan neoian2 aviancarrier nipples danieldaniel Pyrus Bry8Star shadowm_desktop furtardo rdymac TTSDA seaworthy Chiyo yscc Zombiebaron redpill f4cl3y Boohbah applebloom zorael kameloso^ Zetetic XAMPP wheels_up Cuppy-Cake mindlessjohnny Kymru mquin_ Rodja babilen kirin` David Affix jshyeung_ DarkAceZ karakedi\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :jraglin AdriDJ ToApolytoXaos whaletechno jlf Ricardo__ TmvC Sigma00 Casmo breck7 Oldiesmann Rappy naomi thiras moli FRCorey_ iderik glebihan cool_name Dwade09 UniOn eMBee Samual johnnymoo_logsta darknyan mlk dyay xBytez hammond M2Ys4U kobain monoprotic MiLK_ Noldorin njm Nomado Alina-malina abchirk_ Johannes13_ scorche dreamfighter Lars_G DCMT TomyLobo King_Hual No_One fling Mike_H CoreISP djdoody fdd pipitas Subo1977 jef redarrow marcoecc bin_sh TReK\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :[MortiKi] traveller__ Catie DarkG HisaoNakai scounder alone Corycia rudyismydog ToBeFree mcalabrese micadeyeye_ Sembei candlejackson cobra-the-joker ElectricDuck fuzeman swoolley ali_h dungodung oleo brain8675 Jordach rdy4watever KillYourTV coffeee levine m4v dvu ty _nova jgeboski Olipro CheckDavid impulse150 Shadow` jarr0dsz an3k Sove daemon Sary t0rben monkeyjuice Blas alexa the_TORmentor Transfusion kensington Spaceghost wolfmitchell lubmil synick\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :bitnumus krassomat zerox kel39 basiclaser tristero blaxnake themill meznak chinkung DJJeff RBecker XDS2010_ iamblue_cloud excilan Ristovski +JamesTait DrJ pfffx d9b4bef9 Corvus` s0ckpuppet Guest73279 Fritz7 JBreit zinx KhashayaR p3lim_ krisha quackgyver salkaman j4jackj Guest86053 nmmm wiretapped lunchdump goose sam Zarthus jje sl3dge Vutral sins- weie_ +Myrtti _raymond_ KindOne youlysses Mizael jeffz` meet_praveen STalKer-X osxdude surfdue torako\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :niloc132_ connor_goodwolf \\mSg vemacs iViLe slobber zendeavor drbean Tonitrus Nightmare ssbr GorillaPatch TingPing +Pricey james41382 Chenguang Jyothis RansomTime zz_Enviious TakinOver chrisss404 Brando753 mduca SlashLife_ Fuchs DW-Drew Firzen__ Suprano duke johest_ infinem Birdman3131 tmtowtdi Guest61594 BearPerson GiGaHuRtZ Hausas sdamashek salamanderrake jwbirdsong themadhatter mahomet smeggysmeg +kloeri_ kameloso Simonn ryukafalz tigrmesh Borg\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :Kingdom RougeR Venusaur resistivecorpse rush2end destiny rylinaux gskellig Wonkaewt philip quelx hside tann bolt btcvixen joako pr0ggie Xiti` arpita_ Hewn Argus Shippo +Tabmow FrankZZ Olanzapin zhezhe swiatos Argure Cathy JKL1234- Elfix Suicide cali urkl laissez-f sig-wall Guest76477 Rix jok- Guest41925 Thehelpfulone Nimrod oscailt stalled anexus _val_ Luke-Jr Konomi German__ acanete knuth Wildblue` juser Reshesnik Saiban corelax brr JoeK DarkSkyes\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :BullShark Zhaofeng_Li vidplace7 aji APLU RiverRat jerome Bateau raSter^ chipster Vito PigFlu Sprocks evaldoxie Atlas petan javalover MobiusL yerodin Barras2 Barras Humbedooh Niichan SlicerDicer slax0r Brodeles Aurora tandoori Davey Culator|Away gtmanfred kPa ex0a contempt Xack ecks prawnsalad wirehack7 nyuszika7h around Kelsie CaHogan mrpeenut24 Mozart IceCraft LifeIspain andrex sloof thismat troybattey notori0us UbuPhillup lasers BlastHardcheese muskeg\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :archigos +jayne XJR-9 realazthat Geert ahf nim edk spb @ChanServ Azure +nhandler a3li edggeek zol Bigcheese stylus DrBash ingo ningrat zu +tomaw felipe QueenOfFrance dxrt Y_Ichiro sysdef Ju576 rwg [NOT|HERE] sili Snowolf Shnaw tapout Joori GriGore665 LoganCloud Osaka funtapaz MidnighToker codeM0nK3Y Zen kinlo +Corey raj c0ded Remco Vikrant_ xander chadi |L| Shockk nickg ajpiano guntbert funkyHat +erry Mkaysi meingtsla seaLne EvilBlueShark pdelvo Strog\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :geb Vlad Deathspawn mdan Zidonuke jjs999jjs Bry8Star4 inthecompanyof Motzfeldt jlouis mooky avelldiroll tonsofpcs d2r Loki JPT Jamesofur +kloeri Paradox924X Zx3 cruxeternus sdx23 adaptr PwnSauce mattb J21 likewhoa scorche|sh hellome Geek_Juice xorpp Sonar_Guy luckman212 danar psybear eir StarRain sparticus stux|RC-only go|dfish teneightypea BaW d10n riddle Tm_T dive EvilJStoker glowsticks kode54 stwalkerster pocoyo shiftplusone bburhans trucMuche\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :moonlight +yano kevank jmfcool BranchPredictor pppingme Namikaze EvilOne FZTMm Sakaki Lord_Aizen demosdemon JasonDC duracrisis IHateHavingToReg +D[_] AlexJFox Th0masR0ss back Exio4 kunwon1 kc8qvp jeblair Jeruvy Kadet EmLeX aways Kester Spr0cket Thorne csssuf iotku nb solution Zanzibar em mwheeler x56 ChauffeR_ phrozen77 ivan`` The_Cop Monkeh ishanyx Whopper ghz JStoker brabo_ Triskelios sosby PoohBear Clete2 ErrantEgo SebastianFlyte JT jose rubick\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :RDash[AW] wwraith Hazel|artemis mrgaryniger tabeaux crazedpsyc +denny mh0 TheDrums Nothing4You_ Fieldy akawaka thumbs +Dave2 aghos_ Carly-_ Necrosan K1rk ClaudiaU_ HeavyMetal Zenum KOD3N cooldude mshaw milky sepeck Nineain nxp ktr TheLordOfTime dmlloyd sunitknandi arikb pumba Webu `DiM danmackay zomGreg tomboy_ callumacrae Devels rsrx zz_dbRenaud trout Kye Romance _ruben sfan5 brabo lassefaxoe Arieh John13 +ldunn OldSoul|4SiOS501 evilmquin xrated Ishaq\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :phreak turboroot ra roxell topyli jeremias doily Nazca Aehmlo_away nutron +christel +niko DLange drdanick AndrewBarber mediko psk1 TheJH skrip_kid +jbroome +njan RainbowDashh +Plazma vedalken254 codethought md_5 michagogo|cloud brad lolcat +LoRez MissionCritical honzik666 variousnefarious AlanBell tdfischer EricK|AFK AsadH apollo13 Wug[Hyperspace] nullrouted|cloud PeerLesS DarthGandalf cbdev shroud badunkadunk Happzz fortytwo netchip Mike3620 newton\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :Detch Gnumarcoo Brownout Junaos ThalinVien evilErrantEgo Bladerunner +JonathanD Beothoric FloTiX Alenah Raccoon ow GLolol c45y coinspelunk mysteerimasa real_alien tburg SPF|Cloud Cloudiumn like2helpU iMast777 geoffw8__ troyt Hypnotoad nkuttler Sjsws1078_ apoplexy3 trawl AntiSpamMeta ShadowNinja Kernel|Panic vinylGhost GaelanAintAround dlu corentin shark KnownUnown pentiumone133 AimHere Mad7Scientist SaMOOrai Fabianius alamar morphium espiral Someguy123\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :LIP DURgod tehKitten an0nmat1r FuriousRage nanotube jrgifford Mez +gry n4x TDJACR phuzion ohm BradND TheUni OzBorne RumpledElf Internet13 Muzer lostlabyrinth SeySayux midnightmagic drathir Sling firebird +jtrucks Red_M Stary2001 localhost jefferai mosh sweet_kid +RichiH Nothing4You hvxgr FastLizard4 bren2010 Slasher VunKruz sohum MogDog DJones fooly Arokh swords anaconda rcombs Wiretap jeffmjack petteyg TW1920 grawity JakeOrrall mac-mini _Cr4zi3_VM\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :Djole ShapeShifter499 AccessDenied jlcl Jguy sucheta XgF avermedia_ Pyker evil alpha Affliction Spitfire Fohlen rtbt humvee ka6sox benhunter09 mavensk asherkin +Elwell amithkk SolarAquarion chalcedony amarshall mrtux GarethAdams gary_chiang SilentPenguin ebuch_ +jbroome_ TW1920__ LaserShark msimkins Playb3yond music2myear maksbotan tenobi noko eighty4 bitpushr bucketm0use Amrykid phantomcircuit WorldEmperor Reisen pjschmitt armansito piney Yajirobe\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :neuro_sys JordanJ2 z3uS kline Clinteger Taylor albel727 Kharec Rarity Tzunamii VictorRedtail|Sa Peng KWC10 Axew iPod Jasper_Deng_away RyanKnack unreal Haseo aegis mst SecretAgent wapiflapi ghoti _spk_ jeremyb LjL +marienz _TMM_ Archer +gheraint cebor Chris_G Schoentoon jsec Bradford|Nosta addshore cyphase jmbsvicetto liori Plasmastar Skunky chaoscon heinrich5991 nealph catsup SierraAR davidhadas levarnu ping- daurnimator Cr0iX ksx4system Lars_G_ Maple__\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :PcJamesy rej froggyman LanceBNC Vorpal RojoD asakura jaybe Kyle IsoAnon neal__ G ski ibenox Adran Shirik WaffleZ MRX Damage-X Guest90323 jericon irc_adama Nietzschale Mack d1b balrog ikonia GTAXL Michail1 CoJaBo SkyDreamer suborbital Stryyker farn Matrixiumn Fira benonsoftware kaictl jdiez spectra FriendlyFascist Cyclone Koma dwfreed Phoebus jamesd MichaelC|Mobile PennStater SwedFTP +spaceinvader jumperboy Zic Graet ake gbyers[Away] MJ94 keeleysam Dwarf\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :NiTeMaRe arkeet Jake_D alvinek_ debris` Guest13246 infojunky ChrisAM Novacha ImTheBitch capri MartynKeigher BlackoutIsHere WannabeZNC EViLSLuT DrRen KamusHadenes deadpool graphitemaster xy andy_ Cydrobolt Metaleer Oprah Hello71 dirtydawg [Derek] basic` wei2912 nesthib poutine Angelo Simba WormDrink robink zymurgy Guest89644 SirCmpwn enchilado dominikh vivekrai Utility Jason bazhang paddymahoney pinPoint brainproxy TheEpTic Revi N7 Lyude edibsk mb06cs\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :bray90820 IdleOne Console kPa_ shadowm winocm spot digikwondo blishchrot MichaelC swagemon Whiskey win2012 VideoDudeMike HavokOC FailPowah ix007 phenom JZTech101 ohama eric1212 Timbo zz_dlu joey Wooble Willis pseubodot lbft elky BlueShark haxxed JamesOff ndngvr` overrider lahwran plasticboy idoru DXtremz Adonis SeanieB Gizmokid2005 Aerox3 Disori ludkiller dhoss_ c xid b_jonas lurst TheLonelyGod Nietzsche MillHouse Guest19968 AlexP Stoo psycho_oreos G1eb\r\n"
        ":moorcock.freenode.net 353 communi = #freenode :Obfuscate ggherdov dStruct auscompgeek bdfoster tharkun aperson GeordieNorman mfamos irv +tt argv Psi-Jack cups Cprossu TheBadShepperd Magiobiwan mkb Steakanbake three18ti lysobit raztoki Chex Sellyme caf Guest76346 Louis Lexi sa`tan truexfan81 nitrix CodesInChaos Deus N3LRX Tsunamifox tgs3 multiply JakeSaysSays epochwolf totte +t cam daemoneye stump Sargun ekeih tauntaun Milenko vvv upgrayeddd mrrothhcloud___ _anonymous +issyl0 smokex Pici\r\n"
        ":moorcock.freenode.net 366 communi #freenode :End of /NAMES list.\r\n"
        ":ChanServ!ChanServ@services. NOTICE communi :[#freenode] Welcome to #freenode. All network staff are voiced in here, but may not always be around - type /stats p to get a list of on call staff. Others may be hiding so do feel free to ping and /msg us at will! Also please read the channel guidelines at http://freenode.net/poundfreenode.shtml - thanks.\r\n"
        ":services. 328 communi #freenode :http://freenode.net/\r\n";

const char* freenode_names =
        "communi straterra absk007 pefn xlys Gromit TooCool Sambler gat0rs KarneAsada danis_963 Kiryx chrismeller deefloo black_male sxlnxdx bjork Kinny phobos_anomaly T13|sleeps JuxTApose Kolega2357 rorx techhelper1 hermatize Azimi iqualfragile fwilson skasturi mwallacesd mayday Guest76549 mcjohansen MangaKaDenza ARISTIDES ketas `- claptor ylluminate Cooky Brand3n cheater_1 Kirito digitaloktay Will| Iarfen abrotman smurfy Inaunt mist Karol RougeR_ "
        "publickeating An_Ony_Moose michagogo Guest915` davidfg4 Ragnor s1lent_1 keee GingerGeek[Away] hibari derp S_T_A_N anonymuse asantoni road|runner LLckfan neoian2 aviancarrier nipples danieldaniel Pyrus Bry8Star shadowm_desktop furtardo rdymac TTSDA seaworthy Chiyo yscc Zombiebaron redpill f4cl3y Boohbah applebloom zorael kameloso^ Zetetic XAMPP wheels_up Cuppy-Cake mindlessjohnny Kymru mquin_ Rodja babilen kirin` David Affix jshyeung_ DarkAceZ karakedi "
        "jraglin AdriDJ ToApolytoXaos whaletechno jlf Ricardo__ TmvC Sigma00 Casmo breck7 Oldiesmann Rappy naomi thiras moli FRCorey_ iderik glebihan cool_name Dwade09 UniOn eMBee Samual johnnymoo_logsta darknyan mlk dyay xBytez hammond M2Ys4U kobain monoprotic MiLK_ Noldorin njm Nomado Alina-malina abchirk_ Johannes13_ scorche dreamfighter Lars_G DCMT TomyLobo King_Hual No_One fling Mike_H CoreISP djdoody fdd pipitas Subo1977 jef redarrow marcoecc bin_sh TReK "
        "[MortiKi] traveller__ Catie DarkG HisaoNakai scounder alone Corycia rudyismydog ToBeFree mcalabrese micadeyeye_ Sembei candlejackson cobra-the-joker ElectricDuck fuzeman swoolley ali_h dungodung oleo brain8675 Jordach rdy4watever KillYourTV coffeee levine m4v dvu ty _nova jgeboski Olipro CheckDavid impulse150 Shadow` jarr0dsz an3k Sove daemon Sary t0rben monkeyjuice Blas alexa the_TORmentor Transfusion kensington Spaceghost wolfmitchell lubmil synick "
        "bitnumus krassomat zerox kel39 basiclaser tristero blaxnake themill meznak chinkung DJJeff RBecker XDS2010_ iamblue_cloud excilan Ristovski JamesTait DrJ pfffx d9b4bef9 Corvus` s0ckpuppet Guest73279 Fritz7 JBreit zinx KhashayaR p3lim_ krisha quackgyver salkaman j4jackj Guest86053 nmmm wiretapped lunchdump goose sam Zarthus jje sl3dge Vutral sins- weie_ Myrtti _raymond_ KindOne youlysses Mizael jeffz` meet_praveen STalKer-X osxdude surfdue torako "
        "niloc132_ connor_goodwolf \\mSg vemacs iViLe slobber zendeavor drbean Tonitrus Nightmare ssbr GorillaPatch TingPing Pricey james41382 Chenguang Jyothis RansomTime zz_Enviious TakinOver chrisss404 Brando753 mduca SlashLife_ Fuchs DW-Drew Firzen__ Suprano duke johest_ infinem Birdman3131 tmtowtdi Guest61594 BearPerson GiGaHuRtZ Hausas sdamashek salamanderrake jwbirdsong themadhatter mahomet smeggysmeg kloeri_ kameloso Simonn ryukafalz tigrmesh Borg "
        "Kingdom RougeR Venusaur resistivecorpse rush2end destiny rylinaux gskellig Wonkaewt philip quelx hside tann bolt btcvixen joako pr0ggie Xiti` arpita_ Hewn Argus Shippo Tabmow FrankZZ Olanzapin zhezhe swiatos Argure Cathy JKL1234- Elfix Suicide cali urkl laissez-f sig-wall Guest76477 Rix jok- Guest41925 Thehelpfulone Nimrod oscailt stalled anexus _val_ Luke-Jr Konomi German__ acanete knuth Wildblue` juser Reshesnik Saiban corelax brr JoeK DarkSkyes "
        "BullShark Zhaofeng_Li vidplace7 aji APLU RiverRat jerome Bateau raSter^ chipster Vito PigFlu Sprocks evaldoxie Atlas petan javalover MobiusL yerodin Barras2 Barras Humbedooh Niichan SlicerDicer slax0r Brodeles Aurora tandoori Davey Culator|Away gtmanfred kPa ex0a contempt Xack ecks prawnsalad wirehack7 nyuszika7h around Kelsie CaHogan mrpeenut24 Mozart IceCraft LifeIspain andrex sloof thismat troybattey notori0us UbuPhillup lasers BlastHardcheese muskeg "
        "archigos jayne XJR-9 realazthat Geert ahf nim edk spb ChanServ Azure nhandler a3li edggeek zol Bigcheese stylus DrBash ingo ningrat zu tomaw felipe QueenOfFrance dxrt Y_Ichiro sysdef Ju576 rwg [NOT|HERE] sili Snowolf Shnaw tapout Joori GriGore665 LoganCloud Osaka funtapaz MidnighToker codeM0nK3Y Zen kinlo Corey raj c0ded Remco Vikrant_ xander chadi |L| Shockk nickg ajpiano guntbert funkyHat erry Mkaysi meingtsla seaLne EvilBlueShark pdelvo Strog "
        "geb Vlad Deathspawn mdan Zidonuke jjs999jjs Bry8Star4 inthecompanyof Motzfeldt jlouis mooky avelldiroll tonsofpcs d2r Loki JPT Jamesofur kloeri Paradox924X Zx3 cruxeternus sdx23 adaptr PwnSauce mattb J21 likewhoa scorche|sh hellome Geek_Juice xorpp Sonar_Guy luckman212 danar psybear eir StarRain sparticus stux|RC-only go|dfish teneightypea BaW d10n riddle Tm_T dive EvilJStoker glowsticks kode54 stwalkerster pocoyo shiftplusone bburhans trucMuche "
        "moonlight yano kevank jmfcool BranchPredictor pppingme Namikaze EvilOne FZTMm Sakaki Lord_Aizen demosdemon JasonDC duracrisis IHateHavingToReg D[_] AlexJFox Th0masR0ss back Exio4 kunwon1 kc8qvp jeblair Jeruvy Kadet EmLeX aways Kester Spr0cket Thorne csssuf iotku nb solution Zanzibar em mwheeler x56 ChauffeR_ phrozen77 ivan`` The_Cop Monkeh ishanyx Whopper ghz JStoker brabo_ Triskelios sosby PoohBear Clete2 ErrantEgo SebastianFlyte JT jose rubick "
        "RDash[AW] wwraith Hazel|artemis mrgaryniger tabeaux crazedpsyc denny mh0 TheDrums Nothing4You_ Fieldy akawaka thumbs Dave2 aghos_ Carly-_ Necrosan K1rk ClaudiaU_ HeavyMetal Zenum KOD3N cooldude mshaw milky sepeck Nineain nxp ktr TheLordOfTime dmlloyd sunitknandi arikb pumba Webu `DiM danmackay zomGreg tomboy_ callumacrae Devels rsrx zz_dbRenaud trout Kye Romance _ruben sfan5 brabo lassefaxoe Arieh John13 ldunn OldSoul|4SiOS501 evilmquin xrated Ishaq "
        "phreak turboroot ra roxell topyli jeremias doily Nazca Aehmlo_away nutron christel niko DLange drdanick AndrewBarber mediko psk1 TheJH skrip_kid jbroome njan RainbowDashh Plazma vedalken254 codethought md_5 michagogo|cloud brad lolcat LoRez MissionCritical honzik666 variousnefarious AlanBell tdfischer EricK|AFK AsadH apollo13 Wug[Hyperspace] nullrouted|cloud PeerLesS DarthGandalf cbdev shroud badunkadunk Happzz fortytwo netchip Mike3620 newton "
        "Detch Gnumarcoo Brownout Junaos ThalinVien evilErrantEgo Bladerunner JonathanD Beothoric FloTiX Alenah Raccoon ow GLolol c45y coinspelunk mysteerimasa real_alien tburg SPF|Cloud Cloudiumn like2helpU iMast777 geoffw8__ troyt Hypnotoad nkuttler Sjsws1078_ apoplexy3 trawl AntiSpamMeta ShadowNinja Kernel|Panic vinylGhost GaelanAintAround dlu corentin shark KnownUnown pentiumone133 AimHere Mad7Scientist SaMOOrai Fabianius alamar morphium espiral Someguy123 "
        "LIP DURgod tehKitten an0nmat1r FuriousRage nanotube jrgifford Mez gry n4x TDJACR phuzion ohm BradND TheUni OzBorne RumpledElf Internet13 Muzer lostlabyrinth SeySayux midnightmagic drathir Sling firebird jtrucks Red_M Stary2001 localhost jefferai mosh sweet_kid RichiH Nothing4You hvxgr FastLizard4 bren2010 Slasher VunKruz sohum MogDog DJones fooly Arokh swords anaconda rcombs Wiretap jeffmjack petteyg TW1920 grawity JakeOrrall mac-mini _Cr4zi3_VM "
        "Djole ShapeShifter499 AccessDenied jlcl Jguy sucheta XgF avermedia_ Pyker evil alpha Affliction Spitfire Fohlen rtbt humvee ka6sox benhunter09 mavensk asherkin Elwell amithkk SolarAquarion chalcedony amarshall mrtux GarethAdams gary_chiang SilentPenguin ebuch_ jbroome_ TW1920__ LaserShark msimkins Playb3yond music2myear maksbotan tenobi noko eighty4 bitpushr bucketm0use Amrykid phantomcircuit WorldEmperor Reisen pjschmitt armansito piney Yajirobe "
        "neuro_sys JordanJ2 z3uS kline Clinteger Taylor albel727 Kharec Rarity Tzunamii VictorRedtail|Sa Peng KWC10 Axew iPod Jasper_Deng_away RyanKnack unreal Haseo aegis mst SecretAgent wapiflapi ghoti _spk_ jeremyb LjL marienz _TMM_ Archer gheraint cebor Chris_G Schoentoon jsec Bradford|Nosta addshore cyphase jmbsvicetto liori Plasmastar Skunky chaoscon heinrich5991 nealph catsup SierraAR davidhadas levarnu ping- daurnimator Cr0iX ksx4system Lars_G_ Maple__ "
        "PcJamesy rej froggyman LanceBNC Vorpal RojoD asakura jaybe Kyle IsoAnon neal__ G ski ibenox Adran Shirik WaffleZ MRX Damage-X Guest90323 jericon irc_adama Nietzschale Mack d1b balrog ikonia GTAXL Michail1 CoJaBo SkyDreamer suborbital Stryyker farn Matrixiumn Fira benonsoftware kaictl jdiez spectra FriendlyFascist Cyclone Koma dwfreed Phoebus jamesd MichaelC|Mobile PennStater SwedFTP spaceinvader jumperboy Zic Graet ake gbyers[Away] MJ94 keeleysam Dwarf "
        "NiTeMaRe arkeet Jake_D alvinek_ debris` Guest13246 infojunky ChrisAM Novacha ImTheBitch capri MartynKeigher BlackoutIsHere WannabeZNC EViLSLuT DrRen KamusHadenes deadpool graphitemaster xy andy_ Cydrobolt Metaleer Oprah Hello71 dirtydawg [Derek] basic` wei2912 nesthib poutine Angelo Simba WormDrink robink zymurgy Guest89644 SirCmpwn enchilado dominikh vivekrai Utility Jason bazhang paddymahoney pinPoint brainproxy TheEpTic Revi N7 Lyude edibsk mb06cs "
        "bray90820 IdleOne Console kPa_ shadowm winocm spot digikwondo blishchrot MichaelC swagemon Whiskey win2012 VideoDudeMike HavokOC FailPowah ix007 phenom JZTech101 ohama eric1212 Timbo zz_dlu joey Wooble Willis pseubodot lbft elky BlueShark haxxed JamesOff ndngvr` overrider lahwran plasticboy idoru DXtremz Adonis SeanieB Gizmokid2005 Aerox3 Disori ludkiller dhoss_ c xid b_jonas lurst TheLonelyGod Nietzsche MillHouse Guest19968 AlexP Stoo psycho_oreos G1eb "
        "Obfuscate ggherdov dStruct auscompgeek bdfoster tharkun aperson GeordieNorman mfamos irv tt argv Psi-Jack cups Cprossu TheBadShepperd Magiobiwan mkb Steakanbake three18ti lysobit raztoki Chex Sellyme caf Guest76346 Louis Lexi sa`tan truexfan81 nitrix CodesInChaos Deus N3LRX Tsunamifox tgs3 multiply JakeSaysSays epochwolf totte t cam daemoneye stump Sargun ekeih tauntaun Milenko vvv upgrayeddd mrrothhcloud___ _anonymous issyl0 smokex Pici";

const char* freenode_ops = "ChanServ";
const char* freenode_voices = "mist JamesTait Myrtti Pricey kloeri_ Tabmow jayne nhandler tomaw Corey erry kloeri yano D[_] denny Dave2 ldunn christel niko jbroome njan Plazma LoRez JonathanD gry jtrucks RichiH Elwell jbroome_ marienz gheraint spaceinvader tt t issyl0";

void tst_IrcUserModel::testSorting()
{
    if (!serverSocket)
        QSKIP("The address is not available");

    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    waitForWritten(freenode_welcome);
    QCOMPARE(bufferModel.count(), 0);

    waitForWritten(freenode_join);

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    QStringList names = QString::fromUtf8(freenode_names).split(" ");
    QStringList ops = QString::fromUtf8(freenode_ops).split(" ");
    QStringList voices = QString::fromUtf8(freenode_voices).split(" ");

    IrcUserModel userModel(channel);
    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        IrcUser* user = userModel.get(i);
        QCOMPARE(user->name(), names.at(i));
        if (ops.contains(user->name())) {
            QCOMPARE(user->mode(), QString("o"));
            QCOMPARE(user->prefix(), QString("@"));
        } else if (voices.contains(user->name())) {
            QCOMPARE(user->mode(), QString("v"));
            QCOMPARE(user->prefix(), QString("+"));
        }
    }

    QStringList sorted = names;
    qSort(sorted);
    QCOMPARE(userModel.names(), sorted);

    // BY NAME - ASCENDING
    userModel.setSortMethod(Irc::SortByName);
    userModel.sort(0, Qt::AscendingOrder);

    QStringList nasc = names;
    qSort(nasc.begin(), nasc.end(), caseInsensitiveLessThan);

    for (int i = 0; i < userModel.count(); ++i)
        QCOMPARE(userModel.get(i)->name(), nasc.at(i));

    // BY NAME - DESCENDING
    userModel.setSortMethod(Irc::SortByName);
    userModel.sort(0, Qt::DescendingOrder);

    QStringList ndesc = names;
    qSort(ndesc.begin(), ndesc.end(), caseInsensitiveGreaterThan);

    for (int i = 0; i < userModel.count(); ++i)
        QCOMPARE(userModel.get(i)->name(), ndesc.at(i));

    // BY TITLE - ASCENDING
    userModel.setSortMethod(Irc::SortByTitle);
    userModel.sort(0, Qt::AscendingOrder);

    QStringList oasc = ops;
    qSort(oasc.begin(), oasc.end(), caseInsensitiveLessThan);

    QStringList vasc = voices;
    qSort(vasc.begin(), vasc.end(), caseInsensitiveLessThan);

    QStringList titles = oasc + vasc + nasc;
    // remove duplicates
    foreach (const QString& voice, voices)
        titles.removeAt(titles.lastIndexOf(voice));
    foreach (const QString& op, ops)
        titles.removeAt(titles.lastIndexOf(op));

    for (int i = 0; i < userModel.count(); ++i)
        QCOMPARE(userModel.get(i)->name(), titles.at(i));

    // BY TITLE - DESCENDING
    userModel.setSortMethod(Irc::SortByTitle);
    userModel.sort(0, Qt::DescendingOrder);

    QStringList odesc = ops;
    qSort(odesc.begin(), odesc.end(), caseInsensitiveGreaterThan);

    QStringList vdesc = voices;
    qSort(vdesc.begin(), vdesc.end(), caseInsensitiveGreaterThan);

    titles = ndesc + vdesc + odesc;
    // remove duplicates
    foreach (const QString& voice, voices)
        titles.removeAt(titles.indexOf(voice));
    foreach (const QString& op, ops)
        titles.removeAt(titles.indexOf(op));

    for (int i = 0; i < userModel.count(); ++i)
        QCOMPARE(userModel.get(i)->name(), titles.at(i));
}

void tst_IrcUserModel::testActivity()
{
    if (!serverSocket)
        QSKIP("The address is not available");

    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    waitForWritten(freenode_welcome);
    QCOMPARE(bufferModel.count(), 0);

    waitForWritten(freenode_join);

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    QStringList names = QString::fromUtf8(freenode_names).split(" ");

    IrcUserModel activityModel(channel);
    activityModel.setDynamicSort(true);
    activityModel.setSortMethod(Irc::SortByActivity);

    int count = names.count();

    waitForWritten(":smurfy!~smurfy@hidd.en PART #freenode\r\n");
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("smurfy"));

    waitForWritten(":ToApolytoXaos!~ToApolyto@hidd.en QUIT :Quit: Leaving\r\n");
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("ToApolytoXaos"));

    waitForWritten(":agsrv!~guest@hidd.en JOIN #freenode\r\n");
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.user("agsrv")), 0);

    waitForWritten(":Hello71!~Hello71@hidd.en PRIVMSG #freenode :straterra: there are many users on it\r\n");
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.user("Hello71")), 0);

    waitForWritten(":straterra!straterra@hidd.en PRIVMSG #freenode :what?\r\n");
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.user("straterra")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.user("Hello71")), 1);

    waitForWritten(":JuxTApose!~indigital@hidd.en NICK :JuxTApose_afk\r\n");
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("JuxTApose"));
    QCOMPARE(activityModel.indexOf(activityModel.user("JuxTApose_afk")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.user("straterra")), 1);
    QCOMPARE(activityModel.indexOf(activityModel.user("Hello71")), 2);
}

QTEST_MAIN(tst_IrcUserModel)

#include "tst_ircusermodel.moc"
