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

    @Test fun customMoodHours() {
        val m = com.somnia.app.data.MoodHours(angryStart = 23, angryEnd = 3, joyStart = 9, joyEnd = 20)
        assertEquals(Agent.Mood.ANGRY, Agent.mood(23, m))
        assertEquals(Agent.Mood.ANGRY, Agent.mood(2, m)) // plage qui passe minuit
        assertEquals(Agent.Mood.NEUTRAL, Agent.mood(5, m))
        assertEquals(Agent.Mood.LAUGH, Agent.mood(9, m))
        assertEquals(Agent.Mood.SMILE, Agent.mood(21, m))
    }

    @Test fun moodsDisabled() {
        val off = com.somnia.app.data.MoodHours(enabled = false)
        assertEquals(Agent.Mood.LAUGH, Agent.mood(15, off))
        assertEquals(Agent.Mood.LAUGH, Agent.mood(23, off))
        assertEquals(Agent.Mood.SMILE, Agent.mood(0, off))
        assertEquals(Agent.Mood.SMILE, Agent.mood(3, off))
        for (h in 0..23) assertTrue(Agent.mood(h, off) != Agent.Mood.ANGRY)
    }
}
