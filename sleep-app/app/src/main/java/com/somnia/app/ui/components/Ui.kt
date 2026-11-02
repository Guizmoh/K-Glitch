package com.somnia.app.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.ui.draw.shadow
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.ChevronRight
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.ui.theme.BackgroundBrush
import com.somnia.app.ui.theme.Badge
import com.somnia.app.ui.theme.CardStroke
import com.somnia.app.ui.theme.NightCard
import com.somnia.app.ui.theme.TextSoft

private val CardShape = RoundedCornerShape(16.dp)
private val ON_GOLD = Color(0xFF151A33)
/** Reflet de la vitre : plus clair en haut. */
private val GlassSheen = Brush.verticalGradient(listOf(Color(0x24FFFFFF), Color(0x08FFFFFF)))
private val GlassEdge = Brush.verticalGradient(listOf(Color(0x59FFFFFF), Color(0x1AFFFFFF)))

/** Effet vitre teintée : teinte bleu nuit translucide + reflet + liseré clair. */
fun Modifier.tintedGlass(shape: androidx.compose.ui.graphics.Shape = CardShape): Modifier =
    this.clip(shape).background(NightCard).background(GlassSheen).border(1.dp, GlassEdge, shape)

/** Fond uni bleu nuit. */
@Composable
fun NightBackground(content: @Composable BoxScope.() -> Unit) {
    Box(Modifier.fillMaxSize().background(BackgroundBrush), content = content)
}

/** En-tête de page : logo optionnel, titre (empattements) et sous-titre. */
@Composable
fun ScreenHeader(title: String, subtitle: String? = null, logo: Boolean = false, logoAwake: Boolean = true, trailing: @Composable RowScope.() -> Unit = {}) {
    Row(Modifier.fillMaxWidth().padding(top = 16.dp, bottom = 12.dp), verticalAlignment = Alignment.CenterVertically) {
        if (logo) { PoliceLogo(48.dp, awake = logoAwake); Spacer(Modifier.width(14.dp)) }
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.headlineSmall)
            if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodyMedium, color = TextSoft)
        }
        trailing()
    }
}

/** Petit carré d'icône doré. */
@Composable
fun IconBadge(symbol: String?, tint: Color = Badge, size: Dp = 32.dp) {
    val icon = symbolIcon(symbol) ?: return
    Box(Modifier.size(size).clip(RoundedCornerShape(9.dp)).background(tint.copy(alpha = 0.14f)), contentAlignment = Alignment.Center) {
        Icon(icon, null, tint = tint, modifier = Modifier.size(size * 0.58f))
    }
}

/** Carte unie à bord fin. */
@Composable
fun GlassCard(
    modifier: Modifier = Modifier,
    title: String? = null,
    icon: String? = null,
    onClick: (() -> Unit)? = null,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(
        modifier.fillMaxWidth().padding(vertical = 5.dp).clip(CardShape)
            .tintedGlass()
            .then(if (onClick != null) Modifier.clickable(onClick = onClick) else Modifier)
            .padding(16.dp),
    ) {
        if (title != null) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                if (icon != null) { IconBadge(icon); Spacer(Modifier.width(12.dp)) }
                Text(title, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                if (onClick != null) Icon(Icons.Rounded.ChevronRight, null, tint = TextSoft)
            }
            Spacer(Modifier.height(12.dp))
        }
        content()
    }
}

@Composable
fun SectionCard(title: String? = null, modifier: Modifier = Modifier, content: @Composable ColumnScope.() -> Unit) =
    GlassCard(modifier, title, content = content)

/** Carte repliable pour les options avancées. */
@Composable
fun ExpandableCard(title: String, icon: String, subtitle: String? = null, content: @Composable ColumnScope.() -> Unit) {
    var open by remember { mutableStateOf(false) }
    GlassCard(onClick = { open = !open }) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            IconBadge(icon); Spacer(Modifier.width(12.dp))
            Column(Modifier.weight(1f)) {
                Text(title, style = MaterialTheme.typography.titleMedium)
                if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodySmall, color = TextSoft)
            }
            Icon(Icons.Rounded.ChevronRight, null, tint = TextSoft, modifier = Modifier.rotate(if (open) 90f else 0f))
        }
        AnimatedVisibility(open) { Column(Modifier.padding(top = 14.dp)) { content() } }
    }
}

/** Chiffre clé. */
@Composable
fun StatTile(symbol: String, value: String, label: String, modifier: Modifier = Modifier, accent: Color = Color.Unspecified) {
    Column(modifier.tintedGlass().padding(14.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            symbolIcon(symbol)?.let { Icon(it, null, tint = TextSoft, modifier = Modifier.size(16.dp)) }
            Text("  " + label.uppercase(), style = MaterialTheme.typography.labelSmall, color = TextSoft, maxLines = 1)
        }
        Spacer(Modifier.height(8.dp))
        Text(value, style = MaterialTheme.typography.headlineSmall, color = if (accent == Color.Unspecified) MaterialTheme.colorScheme.onSurface else accent, maxLines = 1)
    }
}

@Composable
fun TileRow(content: @Composable RowScope.() -> Unit) =
    Row(Modifier.fillMaxWidth().padding(vertical = 5.dp), horizontalArrangement = Arrangement.spacedBy(10.dp), content = content)

/** Étiquette discrète (avec icône si le texte commence par un symbole connu). */
@Composable
fun Pill(text: String, color: Color, modifier: Modifier = Modifier) {
    val (icon, label) = splitSymbol(text)
    Row(
        modifier.clip(RoundedCornerShape(8.dp)).background(color.copy(alpha = 0.12f)).padding(horizontal = 10.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        icon?.let { Icon(it, null, tint = color, modifier = Modifier.size(15.dp)); Spacer(Modifier.width(6.dp)) }
        Text(label, color = color, style = MaterialTheme.typography.labelLarge)
    }
}

/** Bouton principal : aplat doré. [brush] clair = bouton secondaire crème. */
@Composable
fun GradientButton(text: String, modifier: Modifier = Modifier, brush: Brush = SolidColor(Badge), textColor: Color = ON_GOLD, onClick: () -> Unit) {
    Box(
        modifier.fillMaxWidth().height(54.dp).clip(RoundedCornerShape(14.dp)).background(brush).clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) { Text(text.trim(), color = textColor, fontSize = 16.sp, fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp) }
}

/** Bulle de dialogue (pointe vers le haut, vers l'agent). */
@Composable
fun SpeechBubble(text: String, modifier: Modifier = Modifier) {
    // Bleu clair à 50 % d'opacité : se fond dans le décor, reste lisible
    val cream = Color(0xFFD9E4FA).copy(alpha = 0.66f)
    Column(modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Canvas(Modifier.size(width = 22.dp, height = 11.dp)) {
            val p = androidx.compose.ui.graphics.Path().apply {
                moveTo(size.width / 2, 0f); lineTo(size.width, size.height); lineTo(0f, size.height); close()
            }
            drawPath(p, cream)
        }
        Text(
            text,
            Modifier.clip(RoundedCornerShape(18.dp)).background(cream).padding(horizontal = 18.dp, vertical = 12.dp),
            color = Color(0xFF14204A),
            style = MaterialTheme.typography.titleMedium.copy(fontFamily = androidx.compose.ui.text.font.FontFamily.Serif),
            textAlign = androidx.compose.ui.text.style.TextAlign.Center,
        )
    }
}

/** Bloc d'accueil : l'agent éveillé, sa bulle et le bouton « Commencer la nuit ». */
@Composable
fun BigRoundButton(title: String, subtitle: String, bubble: String? = null, onClick: () -> Unit) {
    Column(
        Modifier.fillMaxWidth().padding(vertical = 8.dp).tintedGlass().padding(20.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        PoliceLogo(132.dp, awake = true)
        Spacer(Modifier.height(6.dp))
        if (bubble != null) SpeechBubble(bubble)
        else Text("L'agent est prêt pour la ronde de nuit.", color = TextSoft, style = MaterialTheme.typography.bodyMedium)
        Spacer(Modifier.height(18.dp))
        Button3D("$title $subtitle", onClick = onClick)
    }
}

/** Gros bouton « en relief » rose bleuté : tranche foncée dessous, reflet dessus, s'enfonce quand on appuie. */
@Composable
fun Button3D(text: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    val src = remember { androidx.compose.foundation.interaction.MutableInteractionSource() }
    val pressed by src.collectIsPressedAsState()
    val depth = if (pressed) 2.dp else 7.dp
    val shape = RoundedCornerShape(22.dp)
    Box(modifier.fillMaxWidth().height(80.dp)) {
        // ombre portée + tranche du bouton
        Box(Modifier.fillMaxWidth().height(73.dp).align(Alignment.BottomCenter)
            .androidxShadow(shape).clip(shape).background(Color(0xFF8A6FB5)))
        // face du bouton
        Box(
            Modifier.fillMaxWidth().height(73.dp).padding(bottom = 0.dp)
                .androidxOffset(7.dp - depth)
                .clip(shape)
                .background(Brush.verticalGradient(listOf(Color(0xFFF0C4E6), Color(0xFFC9AEEE))))
                .border(1.5.dp, Brush.verticalGradient(listOf(Color(0xCCFFFFFF), Color(0x00FFFFFF))), shape)
                .clickable(src, indication = null, onClick = onClick),
            contentAlignment = Alignment.Center,
        ) {
            // reflet en haut
            Box(Modifier.align(Alignment.TopCenter).padding(horizontal = 18.dp, vertical = 6.dp).fillMaxWidth().height(18.dp)
                .clip(RoundedCornerShape(12.dp)).background(Brush.verticalGradient(listOf(Color(0x66FFFFFF), Color(0x00FFFFFF)))))
            Text(text, color = Color(0xFF2B2050), fontSize = 27.sp, fontFamily = com.somnia.app.ui.theme.CozyFont, fontWeight = FontWeight.Bold)
        }
    }
}

private fun Modifier.androidxOffset(y: Dp) = this.then(Modifier.padding(top = 0.dp)).then(androidx.compose.ui.Modifier.offset(y = y))
private fun Modifier.androidxShadow(shape: androidx.compose.ui.graphics.Shape) =
    this.then(androidx.compose.ui.Modifier.shadow(10.dp, shape, ambientColor = Color(0xFF0A1030), spotColor = Color(0xFF0A1030)))

/** Grand titre de l'appli, police ronde et chaleureuse. */
@Composable
fun AppTitle(subtitle: String? = null) {
    Column(Modifier.fillMaxWidth().padding(top = 18.dp, bottom = 10.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        Text(
            "Police du Dodo",
            fontFamily = com.somnia.app.ui.theme.CozyFont, fontWeight = FontWeight.Bold, fontSize = 38.sp,
            color = Color(0xFFF8EFDC),
            style = androidx.compose.ui.text.TextStyle(shadow = androidx.compose.ui.graphics.Shadow(Color(0x660A1030), Offset(0f, 4f), 8f)),
        )
        if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodyMedium, color = TextSoft)
    }
}

/** Pendant l'écoute : anneau fin indiquant le niveau sonore autour du contenu. */
@Composable
fun ListeningOrb(levelDb: Float, floorDb: Float, capturing: Boolean, center: @Composable () -> Unit) {
    val level = ((levelDb - floorDb) / 30f).coerceIn(0f, 1f)
    val color = if (capturing) Color(0xFFE08A7E) else Badge
    Box(Modifier.size(240.dp), contentAlignment = Alignment.Center) {
        Canvas(Modifier.size(224.dp)) {
            val st = 4.dp.toPx()
            val tl = Offset(st / 2, st / 2)
            val sz = Size(size.width - st, size.height - st)
            drawArc(CardStroke, 0f, 360f, false, tl, sz, style = Stroke(st))
            drawArc(color, -90f, 360f * (0.04f + 0.96f * level), false, tl, sz, style = Stroke(st, cap = StrokeCap.Round))
        }
        center()
    }
}

/** Sélecteur segmenté sobre. */
@Composable
fun Segmented(options: List<String>, selected: Int, onSelect: (Int) -> Unit) {
    val shape = RoundedCornerShape(12.dp)
    Row(Modifier.fillMaxWidth().tintedGlass(shape).padding(3.dp)) {
        options.forEachIndexed { i, o ->
            val sel = i == selected
            Box(
                Modifier.weight(1f).clip(RoundedCornerShape(10.dp))
                    .background(if (sel) Color(0x33FFFFFF) else Color.Transparent)
                    .then(if (sel) Modifier.border(1.dp, Badge.copy(alpha = 0.5f), RoundedCornerShape(10.dp)) else Modifier)
                    .clickable { onSelect(i) }.padding(vertical = 10.dp),
                contentAlignment = Alignment.Center,
            ) {
                Text(o, color = if (sel) Badge else TextSoft, fontWeight = FontWeight.SemiBold, fontSize = 15.sp)
            }
        }
    }
}

/** Puce sélectionnable (journal, filtres). */
@Composable
fun ToggleChip(text: String, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(10.dp)
    val (icon, label) = splitSymbol(text)
    Row(
        Modifier.padding(vertical = 4.dp).clip(shape)
            .background(if (selected) Badge.copy(alpha = 0.14f) else Color.Transparent)
            .border(1.dp, if (selected) Badge.copy(alpha = 0.7f) else CardStroke, shape)
            .clickable(onClick = onClick).padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        icon?.let { Icon(it, null, tint = if (selected) Badge else TextSoft, modifier = Modifier.size(16.dp)); Spacer(Modifier.width(6.dp)) }
        Text(label, color = if (selected) Badge else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.9f), fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
    }
}

@Suppress("unused")
private val keepCircle = CircleShape
