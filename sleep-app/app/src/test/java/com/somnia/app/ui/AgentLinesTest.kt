package com.somnia.app.ui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class AgentLinesTest {
    @Test fun messagesByHour() {
        assertEquals("Debout la marmotte !", AgentLines.forHour(8))
        assertEquals("Debout la marmotte !", AgentLines.forHour(11))
        assertEquals("C'est l'heure de la sieste ?", AgentLines.forHour(14))
        assertEquals("C'est l'heure de la sieste ?", AgentLines.forHour(17))
        assertEquals("Bon appétit, Madame !", AgentLines.forHour(18))
        assertEquals("Profitez bien de votre soirée.", AgentLines.forHour(21))
        assertEquals("Profitez bien de votre soirée.", AgentLines.forHour(23))
        assertTrue(AgentLines.forHour(0).startsWith("Première sommation !"))
        assertTrue(AgentLines.forHour(1).startsWith("DEUXIÈME SOMMATION !"))
        assertTrue(AgentLines.forHour(2).contains("prison du Dodo"))
        assertTrue(AgentLines.forHour(4).contains("prison du Dodo"))
        assertTrue(AgentLines.forHour(5).contains("jour de l'an"))
        assertTrue(AgentLines.forHour(7).contains("jour de l'an"))
        for (h in 0..23) assertTrue(AgentLines.forHour(h).isNotBlank())
    }
}
