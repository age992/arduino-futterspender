import { Component } from '@angular/core';
import { Location } from '@angular/common';
import { SettingsService } from 'src/services/settings/settings.service';
import { Settings } from 'src/models/Settings';
import { MatSnackBar } from '@angular/material/snack-bar';
import { ELanguage } from '../../lib/ELanguage';
import { ETheme } from '../../lib/ETheme';

@Component({
  selector: 'settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.scss'],
})
export class SettingsComponent {
  public ELanguage: typeof ELanguage = ELanguage;
  public languageIDs = Object.keys(ELanguage)
    .filter((key) => !isNaN(Number(key)))
    .map((key: string) => Number(key));

  public ETheme: typeof ETheme = ETheme;
  public themeIDs = Object.keys(ETheme)
    .filter((key) => !isNaN(Number(key)))
    .map((key: string) => Number(key));

  settings: Settings | null = null;
  FetchingSettings: boolean = false;
  SavingSettings: boolean = false;

  constructor(
    private location: Location,
    private settingsService: SettingsService,
    private snackBar: MatSnackBar
  ) {
    this.settingsService.Settings.subscribe((s) => {
      this.settings = structuredClone(s);
    });
    this.settingsService.FetchingSettings.subscribe(
      (f) => (this.FetchingSettings = f)
    );
  }

  openSnackBar(message: string) {
    this.snackBar.open(message, 'Ok', { duration: 2000 });
  }

  navBack() {
    this.location.back();
  }

  btnHelp = () => {
    console.log('help dialog');
  };

  save() {
    this.SavingSettings = true;
    console.log(this.settings);

    this.settingsService.updateSettings(this.settings!).subscribe((r) => {
      if (r.status == 200) {
        this.SavingSettings = false;
        this.openSnackBar('Gespeichert!');
        this.navBack();
      } else {
        this.openSnackBar('Fehler beim Speichern der Änderungen!');
        this.SavingSettings = false;
      }
    });
  }
}
