import { HttpResponse } from '@angular/common/http';
import { ReplaySubject, BehaviorSubject, Observable } from 'rxjs';
import { Schedule } from 'src/models/Schedule';

export interface IScheduleService {
  Schedules: ReplaySubject<Schedule[]>;
  Loading: BehaviorSubject<boolean>;
  fetchSchedules(): void;
  upsertSchedule(schedule: Schedule): Observable<HttpResponse<unknown>>;
  deleteSchedule(schedule: Schedule): void;
}
