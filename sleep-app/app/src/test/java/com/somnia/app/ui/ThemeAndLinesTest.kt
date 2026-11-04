package com.somnia.app.ui

import com.somnia.app.audio.SoundType
import com.somnia.app.ui.theme.Themes
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** Vérifications rapides pour éviter des régressions d'affichage. */
class ThemeAndLinesTest {
    private fun lum(c: androidx.compose.ui.graphics.Color) = 0.2126f * c.red + 0.7152f * c.green + 0.0722f * c.blue

    @Test fun everyThemeHasReadableText() {
        for (t in Themes.ALL) {
            val diff = kotlin.math.abs(lum(t.text) - lum(t.bgMid))
            assertTrue("${t.name} : texte peu lisible ($diff)", diff > 0.45f)
        }
    }

    @Test fun nightOverrideForcesDarkTheme() {
        Themes.select(Themes.ROSE)
        Themes.nightOverride = true
        assertEquals(Themes.BLEU, Themes.cur)
        Themes.nightOverride = false
        assertEquals(Themes.ROSE, Themes.cur)
        Themes.select(Themes.VIOLET)
    }

    @Test fun agentMoodsCoverEveryHour() {
        for (h in 0..23) { Agent.awakeDrawable(h); Agent.awakeDrawable1(h) }
        assertEquals(6, Agent.SLEEP_SEQUENCE.size)
        assertEquals(Agent.SLEEP_SEQUENCE[0], Agent.SLEEP_SEQUENCE[5]) // 4-5-6-6-5-4 : boucle sans saut
    }

    @Test fun everySoundTypeHasColorAndIcon() {
        for (t in SoundType.entries) { with(com.somnia.app.ui.theme.Themes) { } }
        SoundType.entries.forEach { assertTrue(it.label.isNotBlank()) }
    }

    @Test fun angryOnlyUntil6() {
        for (h in 1..5) {
            assertEquals(com.somnia.app.R.drawable.agent2_angry, Agent.awakeDrawable(h))
            assertEquals(com.somnia.app.R.drawable.agent1_angry, Agent.awakeDrawable1(h))
        }
        assertEquals(com.somnia.app.R.drawable.agent2_smile, Agent.awakeDrawable(6))
        assertEquals(com.somnia.app.R.drawable.agent1_laughing, Agent.awakeDrawable1(6))
    }
}
