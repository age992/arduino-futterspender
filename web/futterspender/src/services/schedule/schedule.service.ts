import { Injectable } from '@angular/core';
import { IScheduleService } from './schedule.service.interface';
import { Schedule } from 'src/models/Schedule';
import { BehaviorSubject, Observable, ReplaySubject } from 'rxjs';
import { HttpResponse } from '@angular/common/http';

@Injectable({
  providedIn: 'root',
})
export class ScheduleService implements IScheduleService {
  Schedules: ReplaySubject<Schedule[]> = new ReplaySubject();
  Loading: BehaviorSubject<boolean> = new BehaviorSubject(false);

  constructor() {}
  addSchedule(schedule: Schedule): void {
    throw new Error('Method not implemented.');
  }
  updateSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    throw new Error('Method not implemented.');
  }
  deleteSchedule(schedule: Schedule): void {
    throw new Error('Method not implemented.');
  }

  fetchSchedules = () => {};
}
