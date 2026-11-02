package com.somnia.app.ui

import app.cash.paparazzi.DeviceConfig
import app.cash.paparazzi.Paparazzi
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.screens.OnboardingFrame
import com.somnia.app.ui.screens.TUTO_PAGES
import com.somnia.app.ui.theme.SomniaTheme
import org.junit.Assume.assumeTrue
import org.junit.Rule
import org.junit.Test

/**
 * Images du tuto image par image, pour fabriquer le GIF de démonstration.
 * Lancé seulement avec TUTO_FRAMES=1 (./gradlew recordPaparazziDebug --tests '*TutoFrames*').
 */
class TutoFramesTest {
    @get:Rule
    val paparazzi = Paparazzi(deviceConfig = DeviceConfig.PIXEL_6, theme = "android:Theme.Material.NoActionBar", maxPercentDifference = 1.0)

    @Test fun frames() {
        assumeTrue(System.getenv("TUTO_FRAMES") == "1")
        for (p in TUTO_PAGES.indices) {
            for (i in 0 until 24) {
                val t = i * 0.25f
                paparazzi.snapshot("p${p}_f${"%02d".format(i)}") { SomniaTheme { NightBackground { OnboardingFrame(p, t) } } }
            }
        }
    }
}
