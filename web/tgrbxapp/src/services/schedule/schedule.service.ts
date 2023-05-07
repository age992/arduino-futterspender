import { Injectable } from '@angular/core';
import { IScheduleService } from './schedule.service.interface';
import { Schedule } from 'src/models/Schedule';
import { BehaviorSubject, Observable, ReplaySubject } from 'rxjs';
import { HttpResponse } from '@angular/common/http';

@Injectable({
  providedIn: 'root',
})
export class ScheduleService implements IScheduleService {
  Schedules = new BehaviorSubject<Schedule[] | null>(null);
  FetchingSchedules: BehaviorSubject<boolean> = new BehaviorSubject(false);

  constructor() {}
  upsertSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    throw new Error('Method not implemented.');
  }
  deleteSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    throw new Error('Method not implemented.');
  }

  fetchSchedules = () => {};
}
