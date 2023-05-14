import { Injectable } from '@angular/core';
import { ISettingsService } from './settings.service.interface';
import { HttpClient, HttpResponse } from '@angular/common/http';
import { BehaviorSubject, Observable, shareReplay } from 'rxjs';
import { Settings } from 'src/models/Settings';
import { environment } from 'src/environments/environment';

@Injectable({
  providedIn: 'root',
})
export class SettingsService implements ISettingsService {
  Settings = new BehaviorSubject<Settings | null>(null);
  FetchingSettings = new BehaviorSubject<boolean>(false);
  private settingsInternal: Settings | null = null;

  constructor(private http: HttpClient) {
    this.fetchSettings();
  }

  private notify = () => {
    this.Settings.next(this.settingsInternal);
  };

  fetchSettings(): void {
    console.log('Fetch Settings...');
    this.FetchingSettings.next(true);

    this.http
      .get<Settings>(environment.apiUrl + '/settings', {
        observe: 'response',
      })
      .pipe(shareReplay())
      .subscribe((response) => {
        if (response.status == 200) {
          this.settingsInternal = response.body;
          this.FetchingSettings.next(false);
          this.notify();
        }
      });
  }

  updateSettings(updatedSettings: Settings): Observable<HttpResponse<unknown>> {
    console.log('Update settings...');
    const obs = this.http
      .put(environment.apiUrl + '/settings', updatedSettings, {
        observe: 'response',
      })
      .pipe(shareReplay());
    obs.subscribe((response) => {
      if (response.status == 200) {
        this.settingsInternal = updatedSettings;
        this.notify();
      }
    });
    return obs;
  }
}
