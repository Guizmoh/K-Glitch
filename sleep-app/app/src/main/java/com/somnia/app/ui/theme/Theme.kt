package com.somnia.app.ui.theme

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.audio.SoundType

// Palette « Police du Dodo » : bleu nuit, bleu police, or de l'insigne.
val Night = Color(0xFF0B1026)
val NightMid = Color(0xFF151B45)
val NightCard = Color(0x1FFFFFFF)
val CardStroke = Color(0x22FFFFFF)
val PoliceBlue = Color(0xFF5B7CFF)
val PoliceBlueDark = Color(0xFF2B3A8C)
val Badge = Color(0xFFFFC857)
val Moon = Color(0xFFC9BCFF)
val TextSoft = Color(0xFFA9AED6)
val DeepColor = Color(0xFF5B5BD6)
val LightColor = Color(0xFF8FB7FF)
val AwakeColor = Color(0xFFFF9F7A)
val Good = Color(0xFF6EE7A8)

val BackgroundBrush = Brush.verticalGradient(listOf(Color(0xFF1B2160), NightMid, Night))
val PrimaryBrush = Brush.linearGradient(listOf(Color(0xFF6D8BFF), Color(0xFF8E6CFF)))
val SunriseBrush = Brush.linearGradient(listOf(Color(0xFFFFC857), Color(0xFFFF8A65)))

fun SoundType.color(): Color = when (this) {
    SoundType.SPEECH -> Color(0xFF7FD1FF)
    SoundType.NIGHT_TERROR -> Color(0xFFFF6B6B)
    SoundType.SNORING -> Color(0xFFB9A7FF)
    SoundType.LAUGHTER -> Color(0xFFFFD166)
    SoundType.CRYING -> Color(0xFF64B5F6)
    SoundType.COUGH -> Color(0xFF9BE89B)
    SoundType.BREATHING -> Color(0xFF80CBC4)
    SoundType.NOISE -> Color(0xFF9E9EB8)
}

fun scoreColor(score: Int?): Color = when {
    score == null -> Color(0xFF9E9EB8)
    score >= 80 -> Good
    score >= 60 -> Badge
    else -> AwakeColor
}

private val scheme = darkColorScheme(
    primary = PoliceBlue,
    onPrimary = Color.White,
    primaryContainer = PoliceBlueDark,
    onPrimaryContainer = Color.White,
    secondary = Badge,
    onSecondary = Night,
    secondaryContainer = Color(0x33FFC857),
    onSecondaryContainer = Badge,
    background = Night,
    surface = Night,
    surfaceVariant = NightCard,
    surfaceContainer = NightMid,
    surfaceContainerHigh = Color(0xFF1E2558),
    surfaceContainerHighest = Color(0xFF262E66),
    onBackground = Color(0xFFF1F0FF),
    onSurface = Color(0xFFF1F0FF),
    onSurfaceVariant = TextSoft,
    outline = CardStroke,
    outlineVariant = CardStroke,
)

private val typography = Typography().let { t ->
    t.copy(
        displaySmall = t.displaySmall.copy(fontWeight = FontWeight.Bold),
        headlineLarge = t.headlineLarge.copy(fontWeight = FontWeight.Bold),
        headlineMedium = t.headlineMedium.copy(fontWeight = FontWeight.Bold),
        headlineSmall = t.headlineSmall.copy(fontWeight = FontWeight.Bold),
        titleLarge = t.titleLarge.copy(fontWeight = FontWeight.SemiBold),
        titleMedium = t.titleMedium.copy(fontWeight = FontWeight.SemiBold),
        labelSmall = TextStyle(fontSize = 11.sp, letterSpacing = 0.3.sp),
    )
}

private val shapes = Shapes(
    small = RoundedCornerShape(12.dp),
    medium = RoundedCornerShape(20.dp),
    large = RoundedCornerShape(28.dp),
)

@Composable
fun SomniaTheme(content: @Composable () -> Unit) =
    MaterialTheme(colorScheme = scheme, typography = typography, shapes = shapes) {
        // Texte clair par défaut, même hors d'une Surface (fonds dégradés).
        CompositionLocalProvider(LocalContentColor provides scheme.onBackground, content = content)
    }
