using Microsoft.Win32;
using System;
using System.IO;
using System.Text;
using System.Windows;

namespace Installer {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        private string installDir;
        private bool isInstalled = false;

        public MainWindow() {
            installDir = LocateHalfLife();

            if (installDir == null || !Directory.Exists(installDir)) {
                MessageBox.Show("The installer was unable to locate your Half-Life installation.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                Close();
            }

            if (!File.Exists("HLFixes.dll")) {
                MessageBox.Show("The installer was unable to locate HLFixes.dll.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                Close();
            }

            if (!File.Exists($@"{installDir}\hl.exe")) {
                MessageBox.Show($"The installer was unable to locate the Half-Life launcher.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                Close();
            }

            isInstalled = IsLauncherPatched();

            InitializeComponent();
        }

        private string LocateHalfLife() {
            var val = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Valve\Steam").GetValue("ModInstallPath");

            return val != null ? (string)val : null;
        }

        private int FindInArray(byte[] arr, string str) {
            for (int i = 0; i < arr.Length - str.Length; i++) {
                for (int j = 0; j < str.Length; j++) {
                    if (arr[i + j] != str[j]) break;
                    if (j == str.Length - 1) return i;
                }
            }

            return -1;
        }

        private bool IsLauncherPatched() {
            var bytes = File.ReadAllBytes($@"{installDir}\hl.exe");

            return FindInArray(bytes, "hl.fix") != -1;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e) {
            // cheating
            infoLabel.Text = $"Half-Life installation found at: {installDir}    ";
            infoLabel.Text += $"\n\nHLFixes is currently {(isInstalled ? "installed" : "not installed")}.";

            if (isInstalled) {
                installButton.Content = "Reinstall";
                uninstallButton.IsEnabled = true;
            } else {
                installButton.Content = "Install";
                uninstallButton.IsEnabled = false;
            }
        }

        private void installButton_Click(object sender, RoutedEventArgs e) {
            // 1. drop the fix binary
            try {
                File.Copy("HLFixes.dll", $@"{installDir}\hl.fix", true);
            } catch (Exception) {
                MessageBox.Show("The installer was unable write to Half-Life's directory.\nPlease restart the installer as an Administrator.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }
            // 2. locate "hw.dll" in the launcher
            var bytes = File.ReadAllBytes($@"{installDir}\hl.exe");
            var loc = FindInArray(bytes, "hw.dll");

            // 3. patch it to "hl.fix"
            if (loc != -1) {
                var hlfix = Encoding.ASCII.GetBytes("hl.fix");
                for (int i = loc, j = 0; i < loc + hlfix.Length; i++, j++) {
                    bytes[i] = hlfix[j];
                }
            } else if (FindInArray(bytes, "hl.fix") == -1) {
                MessageBox.Show("The installer was unable to patch the Half-Life launcher.\nPlease verify integrity in Steam and try again.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // 4. validate our patch
            if (FindInArray(bytes, "hw.dll") != -1 || FindInArray(bytes, "hl.fix") == -1) {
                MessageBox.Show("The installer was unable to patch the Half-Life launcher.\nPlease verify integrity in Steam and try again.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // 5. write to disk
            try {
                if (!File.Exists($@"{installDir}\hl.exe.bak")) File.Copy($@"{installDir}\hl.exe", $@"{installDir}\hl.exe.bak");
                File.WriteAllBytes($@"{installDir}\hl.exe", bytes);
            } catch (Exception) {
                MessageBox.Show("The installer was unable write to Half-Life's directory.\nPlease restart the installer as an Administrator.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // 6. update the buttons
            if (isInstalled) MessageBox.Show("Successfully reinstalled!", "HLFixes", MessageBoxButton.OK, MessageBoxImage.Information);
            else MessageBox.Show("Successfully installed!", "HLFixes", MessageBoxButton.OK, MessageBoxImage.Information);

            isInstalled = true;
            Window_Loaded(null, null);
        }

        private void uninstallButton_Click(object sender, RoutedEventArgs e) {
            if (!File.Exists($@"{installDir}\hl.exe")) {
                MessageBox.Show("The installer was unable to locate the backup of the Half-Life launcher.\nPlease verify integrity in Steam.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            try {
                File.Copy($@"{installDir}\hl.exe.bak", $@"{installDir}\hl.exe", true);
                File.Delete($@"{installDir}\hl.exe.bak");
                File.Delete($@"{installDir}\hl.fix");
            } catch (Exception) {
                MessageBox.Show("The installer was unable write to Half-Life's directory.\nPlease restart the installer as an Administrator.", "HLFixes Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            MessageBox.Show("Successfully uninstalled!", "HLFixes", MessageBoxButton.OK, MessageBoxImage.Information);

            isInstalled = false;
            Window_Loaded(null, null);
        }
    }
}
