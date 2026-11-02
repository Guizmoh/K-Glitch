package com.somnia.app.ui

import com.somnia.app.ui.theme.uiScale
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class UiScaleTest {
    @Test fun referencePhoneIsOne() = assertEquals(1f, uiScale(411, 914), 0.01f)
    @Test fun smallPhoneShrinks() = assertTrue(uiScale(320, 568) in 0.78f..0.8f)
    @Test fun compactPhoneShrinks() = assertEquals(360 / 411f, uiScale(360, 760), 0.01f)
    @Test fun bigPhoneGrows() = assertTrue(uiScale(480, 1000) > 1.1f)
    @Test fun tabletIsCapped() = assertEquals(1.30f, uiScale(800, 1280), 0.001f)
    @Test fun landscapeUsesPortraitSize() = assertEquals(uiScale(411, 914), uiScale(914, 411), 0.001f)
}
