package com.somnia.app.ui.components

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.Air
import androidx.compose.material.icons.rounded.Alarm
import androidx.compose.material.icons.rounded.Bedtime
import androidx.compose.material.icons.rounded.Campaign
import androidx.compose.material.icons.rounded.DarkMode
import androidx.compose.material.icons.rounded.EditNote
import androidx.compose.material.icons.rounded.Equalizer
import androidx.compose.material.icons.rounded.GraphicEq
import androidx.compose.material.icons.rounded.HelpOutline
import androidx.compose.material.icons.rounded.Hotel
import androidx.compose.material.icons.rounded.HourglassEmpty
import androidx.compose.material.icons.rounded.Insights
import androidx.compose.material.icons.rounded.Lightbulb
import androidx.compose.material.icons.rounded.LocalPolice
import androidx.compose.material.icons.rounded.Lock
import androidx.compose.material.icons.rounded.Mic
import androidx.compose.material.icons.rounded.Palette
import androidx.compose.material.icons.rounded.NotificationsActive
import androidx.compose.material.icons.rounded.RecordVoiceOver
import androidx.compose.material.icons.rounded.SaveAlt
import androidx.compose.material.icons.rounded.SentimentVeryDissatisfied
import androidx.compose.material.icons.rounded.SentimentVerySatisfied
import androidx.compose.material.icons.rounded.Sick
import androidx.compose.material.icons.rounded.Star
import androidx.compose.material.icons.rounded.Tune
import androidx.compose.material.icons.rounded.Tv
import androidx.compose.material.icons.rounded.Visibility
import androidx.compose.material.icons.rounded.VolumeUp
import androidx.compose.material.icons.rounded.Waves
import androidx.compose.material.icons.rounded.WbSunny
import androidx.compose.ui.graphics.vector.ImageVector

/**
 * Les écrans désignent leurs icônes par un symbole court ; on les affiche
 * avec les icônes Material (plus sobres que des emojis).
 */
internal val SYMBOL_ICONS: Map<String, ImageVector> = mapOf(
    "😴" to Icons.Rounded.Bedtime, "🌊" to Icons.Rounded.DarkMode, "👀" to Icons.Rounded.Visibility,
    "🐻" to Icons.Rounded.Waves, "🌙" to Icons.Rounded.Bedtime, "☕" to Icons.Rounded.WbSunny,
    "👮" to Icons.Rounded.LocalPolice, "🔊" to Icons.Rounded.VolumeUp, "🎙️" to Icons.Rounded.Mic,
    "🎛️" to Icons.Rounded.Tune, "💾" to Icons.Rounded.SaveAlt, "⏰" to Icons.Rounded.Alarm,
    "🔔" to Icons.Rounded.NotificationsActive, "🛏️" to Icons.Rounded.Hotel, "🔒" to Icons.Rounded.Lock,
    "📝" to Icons.Rounded.EditNote, "🚨" to Icons.Rounded.GraphicEq, "🎚️" to Icons.Rounded.Equalizer,
    "⭐" to Icons.Rounded.Star, "🔎" to Icons.Rounded.Insights, "⏳" to Icons.Rounded.HourglassEmpty,
    "💡" to Icons.Rounded.Lightbulb, "🎨" to Icons.Rounded.Palette, "📺" to Icons.Rounded.Tv, "🎤" to Icons.Rounded.RecordVoiceOver, "❓" to Icons.Rounded.HelpOutline,
    "🗣️" to Icons.Rounded.RecordVoiceOver, "😱" to Icons.Rounded.Campaign, "😂" to Icons.Rounded.SentimentVerySatisfied,
    "😢" to Icons.Rounded.SentimentVeryDissatisfied, "🤧" to Icons.Rounded.Sick, "💨" to Icons.Rounded.Air,
)

internal fun symbolIcon(s: String?): ImageVector? = s?.let { SYMBOL_ICONS[it] ?: SYMBOL_ICONS[it.removeSuffix("️")] ?: SYMBOL_ICONS[it + "️"] }

/** Sépare un éventuel symbole en tête de texte : "⏰ 07:15" -> (icône Réveil, "07:15"). */
internal fun splitSymbol(text: String): Pair<ImageVector?, String> {
    val first = text.substringBefore(' ')
    val icon = symbolIcon(first)
    return if (icon != null) icon to text.substringAfter(' ').trimStart() else null to text
}
