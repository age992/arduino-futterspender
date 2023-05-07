import { Injectable } from '@angular/core';
import { ISettingsService } from './settings.service.interface';
import { HttpResponse } from '@angular/common/http';
import { BehaviorSubject, Observable } from 'rxjs';
import { Settings } from 'src/models/Settings';

@Injectable({
  providedIn: 'root',
})
export class SettingsService implements ISettingsService {
  Settings = new BehaviorSubject<Settings | null>(null);
  FetchingSettings = new BehaviorSubject<boolean>(false);

  constructor() {}

  fetchSettings(): void {
    throw new Error('Method not implemented.');
  }
  updateSettings(settings: Settings): Observable<HttpResponse<unknown>> {
    throw new Error('Method not implemented.');
  }
}
