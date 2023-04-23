import { Injectable } from '@angular/core';
import { IScheduleService } from './schedule.service.interface';
import { BehaviorSubject, Observable, ReplaySubject } from 'rxjs';
import { Schedule } from 'src/models/Schedule';
import { EScheduleMode } from 'src/app/lib/EScheduleMode';
import { HttpResponse } from '@angular/common/http';
import { schedules } from './MockSchedules';

@Injectable({
  providedIn: 'root',
})
export class ScheduleMockService implements IScheduleService {
  Schedules: ReplaySubject<Schedule[]> = new ReplaySubject();
  Loading: BehaviorSubject<boolean> = new BehaviorSubject(false);

  constructor() {
    this.fetchSchedules();
  }

  addSchedule(schedule: Schedule): void {
    throw new Error('Method not implemented.');
  }

  updateSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    const callback: Observable<HttpResponse<unknown>> = new Observable(
      (subscriber) => {
        setTimeout(() => {
          subscriber.next(new HttpResponse({ status: 200 }));
          subscriber.complete();
        }, 1000);
      }
    );

    callback.subscribe((response) => {
      if (response.status == 200) {
        let i = schedules.findIndex((s) => s.ID == schedule.ID);
        schedules[i] = schedule;
        this.Schedules.next(schedules);
      }
    });
    return callback;
  }

  deleteSchedule(schedule: Schedule): void {
    throw new Error('Method not implemented.');
  }

  fetchSchedules = () => {
    console.log('Update schedules...');
    this.Loading.next(true);
    setTimeout(() => {
      this.Schedules.next(schedules);
      this.Loading.next(false);
    }, 2500);
  };
}
