package com.somnia.app.ui

import com.somnia.app.data.AgentLine

/** Ce que dit l'agent du Dodo sur l'écran d'accueil, selon l'heure (modifiable dans les réglages). */
object AgentLines {
    /** Phrases d'origine. Une plage peut passer minuit (ex. 21 h → 0 h). */
    val DEFAULTS = listOf(
        AgentLine(5, 8, "C'est le jour de l'an ou c'est quoi ce bordel, Madame ?"),
        AgentLine(8, 12, "Debout la marmotte !"),
        AgentLine(12, 14, "Bon déjeuner, Madame !"),
        AgentLine(14, 18, "C'est l'heure de la sieste ?"),
        AgentLine(18, 21, "Bon appétit, Madame !"),
        AgentLine(21, 0, "Profitez bien de votre soirée."),
        AgentLine(0, 1, "Première sommation ! Au lit, ou c'est une amende."),
        AgentLine(1, 2, "DEUXIÈME SOMMATION ! Au lit, ou c'est la prison."),
        AgentLine(2, 5, "Prison du Dodo ! Tout ronflement sera retenu contre vous !"),
    )

    /** Phrase d'origine pour une heure (toujours une phrase). */
    fun forHour(hour: Int): String = forHour(hour, DEFAULTS) ?: ""

    /**
     * Phrase pour [hour] avec les phrases choisies. null = il ne dit rien
     * (aucune phrase à cette heure, ou phrase désactivée, ou texte vide).
     */
    fun forHour(hour: Int, lines: List<AgentLine>): String? {
        val line = lines.firstOrNull { Agent.inRange(hour, it.startHour, it.endHour) } ?: return null
        return line.text.trim().takeIf { line.enabled && it.isNotEmpty() }
    }
}
