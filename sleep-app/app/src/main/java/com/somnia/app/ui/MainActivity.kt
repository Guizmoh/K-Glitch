package com.somnia.app.ui

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.DateRange
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Star
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.somnia.app.ui.screens.HistoryScreen
import com.somnia.app.ui.screens.SessionScreen
import com.somnia.app.ui.screens.SettingsScreen
import com.somnia.app.ui.screens.StatsScreen
import com.somnia.app.ui.screens.TonightScreen
import com.somnia.app.ui.theme.SomniaTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SomniaTheme {
                val nav = rememberNavController()
                val vm: SomniaViewModel = viewModel()
                val entry by nav.currentBackStackEntryAsState()
                val route = entry?.destination?.route
                val tabs = listOf(
                    Triple("tonight", "Cette nuit", Icons.Filled.Star),
                    Triple("history", "Nuits", Icons.Filled.DateRange),
                    Triple("stats", "Tendances", Icons.Filled.Info),
                    Triple("settings", "Réglages", Icons.Filled.Settings),
                )
                Scaffold(
                    bottomBar = {
                        NavigationBar {
                            tabs.forEach { (r, label, icon) ->
                                NavigationBarItem(
                                    selected = route == r,
                                    onClick = {
                                        nav.navigate(r) {
                                            popUpTo(nav.graph.findStartDestination().id) { saveState = true }
                                            launchSingleTop = true
                                            restoreState = true
                                        }
                                    },
                                    icon = { Icon(icon, null) },
                                    label = { Text(label) },
                                )
                            }
                        }
                    },
                ) { pad ->
                    NavHost(nav, startDestination = "tonight", modifier = Modifier.padding(pad)) {
                        composable("tonight") { TonightScreen(vm) { nav.navigate("session/$it") } }
                        composable("history") { HistoryScreen(vm) { nav.navigate("session/$it") } }
                        composable("stats") { StatsScreen(vm) }
                        composable("settings") { SettingsScreen(vm) }
                        composable("session/{id}", listOf(navArgument("id") { type = NavType.LongType })) {
                            SessionScreen(vm, it.arguments?.getLong("id") ?: 0L) { nav.popBackStack() }
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
