# MIPS #

## Sujet ##

Implémenter en C++ le pipeline MIPS étudié en cours. Pour être représentatif de la version hardware du pipeline, ce
dernier devra avoir les propriétés suivantes:

  1. Un thread par étage du pipeline afin de paralléliser leur exécution et rendre indispensable la synchronisation
     des échanges de données entre étages.f
  1. le pipeline est cadancé à la vitesse de l'étage le plus lent. Aucun étage n'est en avance par rapport aux autres,
     c'est-à-dire que le cycle courant est le même pour tous à tout instant.

Un ensemble de classes vous sont fournies afin d'assurer ces propriétés. Votre objectif étant de vous concentrer sur
l'implémentation des étages tels que décrits dans leurs moindres détails dans [les illustrations de cette annexe][1].

La charge de travail du projet est de 3j (2j de dev + 1j de préparation de la soutenance - scripts gdb & exemples).

## Objectifs ##

Vous devez implémenter dans un premier temps la base permettant l'exécution d'instructions indépendantes puis les
forwarding-unit et hazard-detection-unit permettant l'exécution d'instructions avec dépendances. Suivez les
indications/illustrations/explications de la [référence][1] pour réaliser ces étapes. Vos étages doivent contenir
exactement ce qui est visible dans les illustrations (e.g. la control-unit dans ID). Les delay-slots s'obtiennent par
construction en respectant l'architecture du pipeline: les instructions de branchement (e.g. `beq`) impliquent un
delay-slot de deux instructions; la version optimisée à une instruction est acceptée mais n'apporte
aucun point bonus. Enfin, et afin de couvrir tous les cas de gestions des dépendances (software-based interlock vs
hardware-based interlock vs data-forwarding), la hazard-unit de la [référence][1] stall le pipeline (i.e. insère une bulle)
pour gérer un Read-After-Write impliquant l'instruction `lw`.

A vous de choisir quelles instructions vous souhaitez gérer. Choisissez donc (au moins) le sous-ensemble
d'instructions couvrant les objectifs: toutes celles des exemples de la [référence][1], des branchements conditionnels
(e.g. `beq`), des branchements inconditionnels (e.g. `j`) et l'instruction `nop`.

Taillez la mémoire et votre banque de registres en fonctions de vos exemples.

Avant de quitter, votre programme doit écrire sur la sortie standard (au moins) la valeur des registres, le nombre
moyens de cycles par instruction (CPI) et d'instructions par cycle (IPC) nécessaires à l'exécution de votre programme.
Par exemple:

> r1 = 67  
> r2 = 33  
> ...  
> r31 = 2  
> CPI = 5  
> IPC = 0.2

### Soutenance ###

**[Liste de Passage](https://docs.google.com/spreadsheets/d/1_lwp6siRekOX9KWcdfQ6ht9vVxsl4sqo-8_YO-yohRE/edit?usp=sharing)**

La soutenance 2015 est prévue le **13/07/2015** à **Villejuif** (cf. chronos et la liste de passage).
Aucun retard ne sera admis en dehors des plages horaires qui vous sont allouées. Un groupe à l'heure disposera ainsi
de **12 minutes précises** (chronomètre en main) pour présenter son projet.

Chaque groupe vient avec sa machine, son projet et ses exemples prêts pour la soutenance.
Pour chaque groupe:
  1. Exécution de vos exemples.
  1. Démo instrumentée avec GDB de la gestion d'instructions avec dépendances.
  1. Parcours rapide du code source de vos étages afin d'évaluer la fidélité de votre pipeline à la référence.
  
C'est au groupe d'être le moteur de sa soutenance et de montrer chaque point lorsque demandé. N'hésitez pas à fournir
autant d'exemples que vous le souhaitez.

### Notation ###

La grille de notation vous est fournie avec la liste de passage. Voici quelques explications aditionnelles:

- Fidélité: note la fidélité de vos étages par rapport à la réalité (la référence, le cours).
- Latch Struct: note la fidélité de vos structures de latch-results par rapport à la réalité.
- Calcul du CPI/IPC: exécution de programmes MIPS dans votre pipeline à présenter à l'oral et dont les performances
  qui en resulteront (CPI/IPC) seront à expliquer. Dans les cas optimisés, le nombre moyen de cycles par instructions
  diminue et, à l'inverse, le nombre moyen d'instructions par cycle augmente.
    - Exemple de base du pdf #N: illustrations de la [référence][1] à partir de la page 4-12-16.
    - Exemple delay-slot non exploité: exemple de programme non optimisé comportant un ou plusieurs delay-slot
      non exploité(s), c'est-à-dire occupé(s) avec l'instrution `nop`.
    - Exemple delay-slot exploité: exemple **précédent** optimisé. La pertinence de l'exemple est importante.
    - Exemple avec bulles: exemple de programme non optimisé qui génère des stalls (grace à votre
      hazard-detection-unit).
    - Exemple sans bulles: exemple **précédent** optimisé, c'est-à-dire que vous aurez réordonné les instructions
      tel que plus aucun stall n'aura lieu. La pertinence de l'exemple est importante.
- Démo avec scripts GDB: à vous de préparer des scripts de debug permettant d'observer votre
  control/hazard/forward-unit.
  Vous pourriez par exemple insérer un breakpoint dans le cas où vous détectez une dépendance et forwardez des
  données.
  Le but est d'aller le plus vite possible en observant uniquement les cas pertinents.
- Malus: ce qui est indispensable pour les éviter (SMP = multithread).
- Bonus: des points bonus pourront vous être accordées si vous en fait plus qu'attendu.

## Astuces ##

Quelques astuces afin de vous simplifier la tache:

- Simplifiez au maximum le parsing des instructions qui présente peu d'intéret, préférez donc `lw 10 1024 1` à `lw r10, 1024(r1)`.

- Simplifiez les destinations de vos branchement en termes de "numéros de lignes": `j 1` <=> "jumper à la ligne 1". 

- `ifstream` permet de déplacer le curseur de lecture du fichier d'entrée à l'octet prèt. Plutot que de chercher les retours chariots dans vos fichiers afin de déterminer la ligne destination de votre branchement, préférez une solution simple où toutes les lignes ont la meme taille (e.g. 2o caractères dont \n) afin de directement pouvoir déplacer le curseur en début de ligne destination ((ligne courante - ligne destination) * 20).

## Documentation ##

### doc/pipeline-reference.pdf ###

La référence à utiliser pour implémenter le pipeline en C++. Cette dernière contient en plus l'implémentation en VHDL du pipeline. Les illustrations à partir de la page 4-12-16 *More Illustrations of Instruction Execution on the Hardware* sont à suivre à la lettre et constituent en plus vos exemples de base couvrant tous les cas à gérer.

### doc/mips-green-shit.pdf ###

Cheat Sheet de l'ISA MIPS.

## Contenu du repository ##

### src/pipeline.hh ###

Algorithme de pipeline prèt à l'emploi au lieu de la libtbb qui ne répond pas à nos propriétés par défaut.
Cf. src/main.cc ci-dessous pour un exemple d'utilisation.

Vous y trouverez également les classes `p::spinlock` et `p::barrier` qui sont des moyens de synchronisation SMP userland (ce qui permet d'assurer que vos threads seront préemptés en userland et non pas dans un lock kernel - utile pour suivre avec gdb l'exécution de vos étages en les interrompant à tout instant :)).

### src/main.cc ###

Exemple d'utilisation du pipeline et potentiel squelette de base pour votre programme. L'exemple montre comment les latch-results transitent dans le pipeline. Vous pouvez observez ci-dessous ce que donne l'exemple (le numéro après le nom de l'étage est le cycle courant). Vous comprendrez ainsi pourquoi de la synchronisation sera nécessaire lorsque vous ferez transiter des infos entre étages puisque ceux-ci s'executent en parallèle (e.g. cycle 2, EX print avant ID).

```bash
~/mips.git> ./memu.elf test/t2
IF 1: i1
ID 2: i1
IF 2: i2
EX 3: i1
ID 3: i2
EX 4: i2
MEM 4: i1
WB 5: i1
MEM 5: i2
WB 6: i2

Total Number of Cycles = 6
Average IPC = TODO
```

### gdb/ ###
Exemple de script gdb avec debug multi-threading à exploiter en soutenance (et pour votre développement).

### test/ ###
Quelques fichiers de tests qui vont de pair avec l'exemple du repo.

### Makefile.in ###
Spécification de la compilation: flags de compilation nécessaires.

### Bug ###
Cf. le warning dans la fonction `main()` dans `main.cc` qui apparait au moins avec gcc 4.9.0.

### Limitations ###
Un seul appel à `p::pipeline::run()` est possible par instance de la classe `p::pipeline`.  
Patch bienvenu :) Il faut réinitialiser les latchs pour pouvoir relancer le pipeline :)

## Stats ##

### 2014 ###
<img alt="soutenance 2014" src="https://docs.google.com/spreadsheets/d/1uihWUIxEVLblYx8Qz-Mt0GIlq18R0fo5nNCRMoQOLN4/pubchart?oid=387897233&amp;format=image"></div>

[1]: https://github.com/Julio-Guerra/mips/blob/master/doc/pipeline-reference.pdf
