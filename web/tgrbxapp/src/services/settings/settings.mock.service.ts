import { Injectable } from '@angular/core';
import { ISettingsService } from './settings.service.interface';
import { HttpResponse } from '@angular/common/http';
import { BehaviorSubject, Observable } from 'rxjs';
import { Settings } from 'src/models/Settings';
import { settings } from './MockSettings';

@Injectable({
  providedIn: 'root',
})
export class SettingsMockService implements ISettingsService {
  Settings = new BehaviorSubject<Settings | null>(null);
  FetchingSettings = new BehaviorSubject<boolean>(false);

  private settingsInternal: Settings | null = null;

  constructor() {
    this.fetchSettings();
  }

  private notify = () => {
    this.Settings.next(this.settingsInternal);
  };

  fetchSettings = () => {
    console.log('Fetch settings...');
    this.FetchingSettings.next(true);
    setTimeout(() => {
      this.settingsInternal = settings;
      this.FetchingSettings.next(false);
      this.notify();
    }, 2500);
  };

  updateSettings(settings: Settings): Observable<HttpResponse<unknown>> {
    const clonedSettings = structuredClone(settings);

    const callback: Observable<HttpResponse<unknown>> = new Observable(
      (subscriber) => {
        setTimeout(() => {
          subscriber.next(new HttpResponse({ status: 200 }));
          subscriber.complete();
        }, 2000);
      }
    );

    callback.subscribe((response) => {
      if (response.status == 200) {
        this.settingsInternal = clonedSettings;
        this.notify();
      }
    });

    return callback;
  }
}
