/*
	Ce module doit g�rer la liste des personnes pr�sentes sur le minichat.

	<cLx> �a te dirait qu'on se fasse un module qui s'appelle nicklist.c, qui va prendre les appels de parsehtml.c, les requetes de whois de mccirc et qui va aussi fournir la nicklist pour gotcurses.c ?
	<niki> �a serait bien
	<cLx> btw, a chaque message, �a va appeler une fonction dans nicklist.c pour mettre � jour l'url de l'icone, du profil, le time du dernier message dit ...
	<cLx> si une personne est invisible mais parle quand m�me, on va quand m�me l'ajouter � la liste d'ailleurs avec un flag pour dire qu'elle est invisible et qu'il faudra compter sur un timeout depuis le dernier message pour la virer de la liste ou pas

	elements de la "base de donnees" :
		- nickname
		- l'url de son ic�ne (r�cup�r�e quand la personne parle par msg->usericonurl)
		- l'url du profil (r�cup�r�e quand la personne parle par msg->userprofileurl)
		- une petite chaine pour y mettre un ident dedans (pour etre utilis�e comme mask apr�s)
		- un timestamp qui indique la date et l'heure d'ajout � la nicklist
		- un timestamp qui indique la date et l'heure du dernier message de la personne
		- un timestamp qui indique la date et l'heure de la derni�re fois qu'on a vu la personne dans la liste de gens sur le minichat
		- un tag qui indique si la personne � �t� rajout�e � la nicklist sans qu'elle apparaisse dans la liste (ie: elle est invisible) ? En option, puisqu'il suffit de v�rifier si le timestamp pr�c�dent est � 0 ou pas en fait...
*/


