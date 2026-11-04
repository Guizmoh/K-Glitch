package com.somnia.app.ui

/** Ce que dit l'agent du Dodo sur l'écran d'accueil, selon l'heure. */
object AgentLines {
    fun forHour(hour: Int): String = when (hour) {
        in 5..7 -> "C'est le jour de l'an ou c'est quoi ce bordel, Madame ?"
        in 8..11 -> "Debout la marmotte !"
        in 12..13 -> "Bon déjeuner, Madame !"
        in 14..17 -> "C'est l'heure de la sieste ?"
        in 18..20 -> "Bon appétit, Madame !"
        in 21..23 -> "Profitez bien de votre soirée."
        0 -> "Première sommation ! Au lit, ou c'est une amende."
        1 -> "DEUXIÈME SOMMATION ! Au lit, ou c'est la prison."
        else -> "Prison du Dodo ! Tout ronflement sera retenu contre vous !"
    }
}
