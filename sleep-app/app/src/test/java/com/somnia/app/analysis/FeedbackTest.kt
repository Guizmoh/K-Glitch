package com.somnia.app.analysis

import com.somnia.app.audio.SoundType
import com.somnia.app.data.Settings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class FeedbackTest {
    @Test fun falseLaughterLowersLaughter() {
        val s = Settings()
        val (n, msg) = Feedback.adjust(s, SoundType.LAUGHTER, SoundType.NOISE)
        assertEquals(s.typeSensitivity[SoundType.LAUGHTER]!! - 1, n.typeSensitivity[SoundType.LAUGHTER])
        assertEquals("Réglage ajusté : Rire 6 → 5", msg)
    }

    @Test fun swapLowersWrongAndRaisesRight() {
        val (n, _) = Feedback.adjust(Settings(), SoundType.SPEECH, SoundType.CRYING)
        assertEquals(5, n.typeSensitivity[SoundType.SPEECH])
        assertEquals(7, n.typeSensitivity[SoundType.CRYING])
    }

    @Test fun boundsAreKept() {
        val s = Settings(typeSensitivity = Settings.defaultTypeSensitivity() + (SoundType.COUGH to 1))
        val (n, msg) = Feedback.adjust(s, SoundType.COUGH, SoundType.NOISE)
        assertEquals(1, n.typeSensitivity[SoundType.COUGH])
        assertNull(msg)
    }
}
