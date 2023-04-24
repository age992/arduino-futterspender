import { HttpResponse } from '@angular/common/http';
import { BehaviorSubject, Observable } from 'rxjs';
import { Settings } from 'src/models/Settings';

export interface ISettingsService {
  Settings: BehaviorSubject<Settings | null>;
  FetchingSettings: BehaviorSubject<boolean>;
  fetchSettings(): void;
  updateSettings(settings: Settings): Observable<HttpResponse<unknown>>;
}
