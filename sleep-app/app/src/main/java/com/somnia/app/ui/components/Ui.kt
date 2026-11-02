package com.somnia.app.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
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
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
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
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.R
import com.somnia.app.ui.theme.BackgroundBrush
import com.somnia.app.ui.theme.CardStroke
import com.somnia.app.ui.theme.NightCard
import com.somnia.app.ui.theme.PrimaryBrush
import com.somnia.app.ui.theme.TextSoft

/** Fond dégradé « ciel de nuit » avec quelques étoiles. */
@Composable
fun NightBackground(content: @Composable BoxScope.() -> Unit) {
    Box(Modifier.fillMaxSize().background(BackgroundBrush)) {
        Canvas(Modifier.fillMaxSize()) {
            val stars = listOf(0.12f to 0.06f, 0.82f to 0.04f, 0.65f to 0.14f, 0.3f to 0.2f, 0.92f to 0.27f, 0.05f to 0.33f, 0.5f to 0.09f)
            stars.forEachIndexed { i, (x, y) ->
                drawCircle(Color.White.copy(alpha = 0.25f + 0.1f * (i % 3)), radius = (1.2f + i % 2) * density, center = androidx.compose.ui.geometry.Offset(x * size.width, y * size.height))
            }
        }
        content()
    }
}

/** Logo : le policier du sommeil dans un médaillon. */
@Composable
fun PoliceLogo(size: Dp = 56.dp) {
    Box(
        Modifier.size(size).clip(CircleShape)
            .background(Brush.linearGradient(listOf(Color(0xFF3B4FC4), Color(0xFF141A4A))))
            .border(1.dp, Color.White.copy(alpha = 0.18f), CircleShape),
        contentAlignment = Alignment.Center,
    ) {
        Image(painterResource(R.drawable.ic_launcher_foreground), "Police du Dodo", Modifier.size(size * 1.75f).offset(y = size * 0.04f))
    }
}

/** En-tête de page : logo optionnel, titre et sous-titre. */
@Composable
fun ScreenHeader(title: String, subtitle: String? = null, logo: Boolean = false, trailing: @Composable RowScope.() -> Unit = {}) {
    Row(Modifier.fillMaxWidth().padding(top = 12.dp, bottom = 8.dp), verticalAlignment = Alignment.CenterVertically) {
        if (logo) { PoliceLogo(48.dp); Spacer(Modifier.width(12.dp)) }
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.headlineSmall)
            if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodyMedium, color = TextSoft)
        }
        trailing()
    }
}

/** Carte translucide arrondie (effet « verre »). */
@Composable
fun GlassCard(
    modifier: Modifier = Modifier,
    title: String? = null,
    icon: String? = null,
    onClick: (() -> Unit)? = null,
    content: @Composable ColumnScope.() -> Unit,
) {
    val shape = RoundedCornerShape(24.dp)
    Column(
        modifier.fillMaxWidth().padding(vertical = 6.dp).clip(shape)
            .background(NightCard).border(1.dp, CardStroke, shape)
            .then(if (onClick != null) Modifier.clickable(onClick = onClick) else Modifier)
            .padding(18.dp),
    ) {
        if (title != null) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                if (icon != null) Text(icon, fontSize = 18.sp, modifier = Modifier.padding(end = 8.dp))
                Text(title, style = MaterialTheme.typography.titleMedium)
            }
            Spacer(Modifier.height(10.dp))
        }
        content()
    }
}

/** Ancien nom conservé pour les écrans existants. */
@Composable
fun SectionCard(title: String? = null, modifier: Modifier = Modifier, content: @Composable ColumnScope.() -> Unit) =
    GlassCard(modifier, title, content = content)

/** Carte repliable pour les options avancées. */
@Composable
fun ExpandableCard(title: String, icon: String, subtitle: String? = null, content: @Composable ColumnScope.() -> Unit) {
    var open by remember { mutableStateOf(false) }
    GlassCard(onClick = { open = !open }) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(icon, fontSize = 18.sp, modifier = Modifier.padding(end = 8.dp))
            Column(Modifier.weight(1f)) {
                Text(title, style = MaterialTheme.typography.titleMedium)
                if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodySmall, color = TextSoft)
            }
            Text("›", fontSize = 26.sp, color = TextSoft, modifier = Modifier.rotate(if (open) 90f else 0f))
        }
        AnimatedVisibility(open) { Column(Modifier.padding(top = 12.dp)) { content() } }
    }
}

/** Petite tuile de chiffre clé. */
@Composable
fun StatTile(emoji: String, value: String, label: String, modifier: Modifier = Modifier, accent: Color = Color.White) {
    val shape = RoundedCornerShape(20.dp)
    Column(
        modifier.clip(shape).background(NightCard).border(1.dp, CardStroke, shape).padding(14.dp),
    ) {
        Text(emoji, fontSize = 20.sp)
        Spacer(Modifier.height(6.dp))
        Text(value, style = MaterialTheme.typography.titleLarge, color = accent, maxLines = 1)
        Text(label, style = MaterialTheme.typography.labelSmall, color = TextSoft, maxLines = 1)
    }
}

@Composable
fun TileRow(content: @Composable RowScope.() -> Unit) =
    Row(Modifier.fillMaxWidth().padding(vertical = 6.dp), horizontalArrangement = Arrangement.spacedBy(10.dp), content = content)

/** Pastille colorée (compteur de sons, étiquette...). */
@Composable
fun Pill(text: String, color: Color, modifier: Modifier = Modifier) {
    Text(
        text,
        modifier.clip(RoundedCornerShape(50)).background(color.copy(alpha = 0.18f)).padding(horizontal = 12.dp, vertical = 6.dp),
        color = color, style = MaterialTheme.typography.labelLarge,
    )
}

/** Bouton principal pleine largeur en dégradé. */
@Composable
fun GradientButton(text: String, modifier: Modifier = Modifier, brush: Brush = PrimaryBrush, textColor: Color = Color.White, onClick: () -> Unit) {
    Box(
        modifier.fillMaxWidth().height(60.dp).clip(RoundedCornerShape(30.dp)).background(brush).clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) { Text(text, color = textColor, fontSize = 17.sp, fontWeight = FontWeight.SemiBold) }
}

/** Gros bouton rond « Commencer la nuit » avec halo qui respire. */
@Composable
fun BigRoundButton(title: String, subtitle: String, onClick: () -> Unit) {
    val t = rememberInfiniteTransition(label = "halo")
    val pulse by t.animateFloat(1f, 1.08f, infiniteRepeatable(tween(2200, easing = LinearEasing), RepeatMode.Reverse), label = "p")
    Box(Modifier.size(250.dp), contentAlignment = Alignment.Center) {
        Box(Modifier.size(230.dp).scale(pulse).clip(CircleShape).background(Color(0x226D8BFF)))
        Box(Modifier.size(200.dp).scale(pulse * 0.98f).clip(CircleShape).background(Color(0x336D8BFF)))
        Column(
            Modifier.size(170.dp).clip(CircleShape).background(PrimaryBrush).clickable(onClick = onClick),
            horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.Center,
        ) {
            Text("🌙", fontSize = 40.sp)
            Text(title, color = Color.White, fontWeight = FontWeight.Bold, fontSize = 18.sp, textAlign = TextAlign.Center)
            Text(subtitle, color = Color.White.copy(alpha = 0.8f), fontSize = 12.sp)
        }
    }
}

/** Orbe animé qui grossit avec le niveau sonore pendant l'écoute. */
@Composable
fun ListeningOrb(levelDb: Float, floorDb: Float, capturing: Boolean, center: @Composable () -> Unit) {
    val t = rememberInfiniteTransition(label = "orb")
    val breathe by t.animateFloat(0.96f, 1.04f, infiniteRepeatable(tween(3000), RepeatMode.Reverse), label = "b")
    val boost = ((levelDb - floorDb) / 30f).coerceIn(0f, 0.25f)
    val color = if (capturing) Color(0xFFFF8A80) else Color(0xFF6D8BFF)
    Box(Modifier.size(260.dp), contentAlignment = Alignment.Center) {
        Box(Modifier.size(240.dp).scale(breathe + boost).clip(CircleShape).background(color.copy(alpha = 0.12f)))
        Box(Modifier.size(205.dp).scale(breathe + boost / 2).clip(CircleShape).background(color.copy(alpha = 0.2f)))
        Box(
            Modifier.size(175.dp).clip(CircleShape)
                .background(Brush.radialGradient(listOf(color.copy(alpha = 0.55f), Color(0xFF1B2160))))
                .border(1.dp, Color.White.copy(alpha = 0.2f), CircleShape),
            contentAlignment = Alignment.Center,
        ) { center() }
    }
}

/** Sélecteur segmenté (ex. modes de captation). */
@Composable
fun Segmented(options: List<String>, selected: Int, onSelect: (Int) -> Unit) {
    val shape = RoundedCornerShape(50)
    Row(Modifier.fillMaxWidth().clip(shape).background(Color(0x14FFFFFF)).padding(4.dp)) {
        options.forEachIndexed { i, o ->
            val sel = i == selected
            Box(
                Modifier.weight(1f).clip(shape)
                    .background(if (sel) PrimaryBrush else Brush.linearGradient(listOf(Color.Transparent, Color.Transparent)))
                    .clickable { onSelect(i) }.padding(vertical = 10.dp),
                contentAlignment = Alignment.Center,
            ) {
                Text(o, color = if (sel) Color.White else TextSoft, fontWeight = if (sel) FontWeight.SemiBold else FontWeight.Normal, fontSize = 14.sp)
            }
        }
    }
}

/** Puce sélectionnable légère (journal). */
@Composable
fun ToggleChip(text: String, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(50)
    Text(
        text,
        Modifier.padding(vertical = 4.dp).clip(shape)
            .background(if (selected) Color(0x445B7CFF) else Color(0x10FFFFFF))
            .border(1.dp, if (selected) Color(0xFF6D8BFF) else CardStroke, shape)
            .clickable(onClick = onClick).padding(horizontal = 14.dp, vertical = 8.dp),
        color = if (selected) Color.White else TextSoft, fontSize = 14.sp,
    )
}
