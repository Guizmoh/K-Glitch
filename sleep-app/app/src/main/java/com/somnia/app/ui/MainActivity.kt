package com.somnia.app.ui

import android.graphics.Color as AColor
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.somnia.app.ui.screens.OnboardingScreen
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.screens.HistoryScreen
import com.somnia.app.ui.screens.SessionScreen
import com.somnia.app.ui.screens.SettingsScreen
import com.somnia.app.ui.screens.StatsScreen
import com.somnia.app.ui.screens.TonightScreen
import com.somnia.app.ui.theme.PoliceBlueDark
import com.somnia.app.ui.theme.SomniaTheme
import com.somnia.app.ui.theme.TextSoft

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.dark(AColor.TRANSPARENT),
            navigationBarStyle = SystemBarStyle.dark(AColor.TRANSPARENT),
        )
        setContent {
            SomniaTheme {
                val nav = rememberNavController()
                val vm: SomniaViewModel = viewModel()
                val entry by nav.currentBackStackEntryAsState()
                val route = entry?.destination?.route
                val tabs = listOf(
                    Triple("tonight", "Ce soir", "🌙"),
                    Triple("history", "Mes nuits", "📋"),
                    Triple("stats", "Tendances", "📈"),
                    Triple("settings", "Réglages", "⚙️"),
                )
                val prefs = remember { getSharedPreferences("app", MODE_PRIVATE) }
                var showTuto by remember { mutableStateOf(!prefs.getBoolean("tuto_done", false)) }
                NightBackground {
                    if (showTuto) {
                        OnboardingScreen { prefs.edit().putBoolean("tuto_done", true).apply(); showTuto = false }
                        return@NightBackground
                    }
                    Scaffold(
                        containerColor = Color.Transparent,
                        bottomBar = {
                            NavigationBar(containerColor = Color(0xE60B1026), tonalElevation = 0.dp) {
                                tabs.forEach { (r, label, emoji) ->
                                    val selected = route == r || (r == "history" && route?.startsWith("session") == true)
                                    NavigationBarItem(
                                        selected = selected,
                                        onClick = {
                                            nav.navigate(r) {
                                                popUpTo(nav.graph.findStartDestination().id) { saveState = true }
                                                launchSingleTop = true
                                                restoreState = true
                                            }
                                        },
                                        icon = { Text(emoji, fontSize = 22.sp) },
                                        label = { Text(label) },
                                        colors = NavigationBarItemDefaults.colors(
                                            indicatorColor = PoliceBlueDark,
                                            selectedTextColor = Color.White,
                                            unselectedTextColor = TextSoft,
                                        ),
                                    )
                                }
                            }
                        },
                    ) { pad ->
                        NavHost(
                            nav, startDestination = "tonight", modifier = Modifier.padding(pad),
                            enterTransition = { fadeIn() }, exitTransition = { fadeOut() },
                        ) {
                            composable("tonight") { TonightScreen(vm) { nav.navigate("session/$it") } }
                            composable("history") { HistoryScreen(vm) { nav.navigate("session/$it") } }
                            composable("stats") { StatsScreen(vm) }
                            composable("settings") { SettingsScreen(vm) { showTuto = true } }
                            composable("session/{id}", listOf(navArgument("id") { type = NavType.LongType })) {
                                SessionScreen(vm, it.arguments?.getLong("id") ?: 0L) { nav.popBackStack() }
                            }
                        }
                    }
                }
            }
        }
    }

    override fun onStop() {
        super.onStop()
        AudioPlayer.stop()
    }
}

