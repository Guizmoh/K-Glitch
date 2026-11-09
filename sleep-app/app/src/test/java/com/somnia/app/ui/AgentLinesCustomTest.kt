package com.somnia.app.ui

import com.somnia.app.data.AgentLine
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class AgentLinesCustomTest {
    @Test fun customLinesAndSilence() {
        val lines = listOf(
            AgentLine(22, 2, "Au lit !"),                 // passe minuit
            AgentLine(9, 12, "Bonjour", enabled = false), // désactivée : il ne dit rien
            AgentLine(14, 16, "   "),                     // vide : rien
        )
        assertEquals("Au lit !", AgentLines.forHour(23, lines))
        assertEquals("Au lit !", AgentLines.forHour(1, lines))
        assertNull(AgentLines.forHour(10, lines))
        assertNull(AgentLines.forHour(15, lines))
        assertNull(AgentLines.forHour(18, lines)) // aucune phrase à cette heure
    }

    @Test fun defaultsCoverEveryHour() {
        for (h in 0..23) assert(AgentLines.forHour(h, AgentLines.DEFAULTS) != null) { "rien à $h h" }
    }
}
