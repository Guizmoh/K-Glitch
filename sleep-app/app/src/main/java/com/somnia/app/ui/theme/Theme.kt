package com.somnia.app.ui.theme

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.*
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
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.audio.SoundType

// Palette « Police du Dodo » : bleu nuit, bleu police, or de l'insigne.
val Night = Color(0xFF213268)
val NightMid = Color(0xFF283C7A)
val NightCard = Color(0x5A16234F) // vitre teintée semi-opaque
val CardStroke = Color(0x3DFFFFFF)
val PoliceBlue = Color(0xFFE6C47A) // or de l'insigne (couleur d'accent)
val PoliceBlueDark = Color(0xFF2A3360)
val Badge = Color(0xFFE6C47A)
val Moon = Color(0xFFC9BCFF)
val TextSoft = Color(0xFFB9C1DE)
val DeepColor = Color(0xFF4A5AA8)
val LightColor = Color(0xFF8FB7FF)
val AwakeColor = Color(0xFFE3A07F)
val Good = Color(0xFF8FCFA5)

val BackgroundBrush = Brush.verticalGradient(listOf(Color(0xFF34509A), Color(0xFF283E80), Night))
val PrimaryBrush = Brush.linearGradient(listOf(Color(0xFFE6C47A), Color(0xFFE6C47A)))
val SunriseBrush = Brush.linearGradient(listOf(Color(0xFFF1E6CF), Color(0xFFF1E6CF)))

fun SoundType.color(): Color = when (this) {
    SoundType.SPEECH -> Color(0xFF8DB8E0)
    SoundType.NIGHT_TERROR -> Color(0xFFE08A7E)
    SoundType.SNORING -> Color(0xFFB3A6D9)
    SoundType.LAUGHTER -> Color(0xFFE6C47A)
    SoundType.CRYING -> Color(0xFF7FA7D6)
    SoundType.COUGH -> Color(0xFF9CC9A3)
    SoundType.BREATHING -> Color(0xFF8CC2BC)
    SoundType.NOISE -> Color(0xFF9AA2C0)
}

fun scoreColor(score: Int?): Color = when {
    score == null -> Color(0xFF9E9EB8)
    score >= 80 -> Good
    score >= 60 -> Badge
    else -> AwakeColor
}

private val scheme = darkColorScheme(
    primary = PoliceBlue,
    onPrimary = Color(0xFF151A33),
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
    surfaceContainerHigh = Color(0xFF2C4084),
    surfaceContainerHighest = Color(0xFF34498F),
    onBackground = Color(0xFFEDE8DC),
    onSurface = Color(0xFFEDE8DC),
    onSurfaceVariant = TextSoft,
    outline = CardStroke,
    outlineVariant = CardStroke,
)

private val typography = Typography().let { t ->
    t.copy(
        // Titres à empattements, comme le titre de l'affiche
        displaySmall = t.displaySmall.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineLarge = t.headlineLarge.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineMedium = t.headlineMedium.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineSmall = t.headlineSmall.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        titleLarge = t.titleLarge.copy(fontWeight = FontWeight.SemiBold),
        titleMedium = t.titleMedium.copy(fontWeight = FontWeight.Medium),
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
        // Texte agrandi d'environ 12 % (en plus du réglage de taille du téléphone).
        val d = androidx.compose.ui.platform.LocalDensity.current
        CompositionLocalProvider(
            LocalContentColor provides scheme.onBackground,
            androidx.compose.ui.platform.LocalDensity provides androidx.compose.ui.unit.Density(d.density, d.fontScale * 1.12f),
            content = content,
        )
    }

fun SoundType.icon(): androidx.compose.ui.graphics.vector.ImageVector = when (this) {
    SoundType.SPEECH -> Icons.Rounded.RecordVoiceOver
    SoundType.NIGHT_TERROR -> Icons.Rounded.Campaign
    SoundType.SNORING -> Icons.Rounded.Waves
    SoundType.LAUGHTER -> Icons.Rounded.SentimentVerySatisfied
    SoundType.CRYING -> Icons.Rounded.SentimentVeryDissatisfied
    SoundType.COUGH -> Icons.Rounded.Sick
    SoundType.BREATHING -> Icons.Rounded.Air
    SoundType.NOISE -> Icons.Rounded.GraphicEq
}
