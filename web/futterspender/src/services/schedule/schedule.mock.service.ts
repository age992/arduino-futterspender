import { Injectable } from '@angular/core';
import { IScheduleService } from './schedule.service.interface';
import { BehaviorSubject, Observable, ReplaySubject } from 'rxjs';
import { Schedule } from 'src/models/Schedule';
import { HttpResponse } from '@angular/common/http';
import { schedules } from './MockSchedules';

@Injectable({
  providedIn: 'root',
})
export class ScheduleMockService implements IScheduleService {
  Schedules: ReplaySubject<Schedule[]> = new ReplaySubject();
  Loading: BehaviorSubject<boolean> = new BehaviorSubject(false);

  private schedulesInternal: Schedule[] = [];

  constructor() {
    this.fetchSchedules();
  }

  private notify = () => {
    this.Schedules.next(this.schedulesInternal);
  };

  upsertSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    const isUpdate = schedule.ID;

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
        let i = !isUpdate
          ? -1
          : this.schedulesInternal.findIndex((s) => s.ID == schedule.ID);
        if (i != -1) {
          this.schedulesInternal[i] = schedule;
        } else {
          this.schedulesInternal.push(schedule);
        }
        this.notify();
      }
    });

    return callback;
  }

  deleteSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
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
        let i = this.schedulesInternal.findIndex((s) => s.ID == schedule.ID);
        if (i != -1) {
          const a = this.schedulesInternal.slice(0, i);
          const b = this.schedulesInternal.slice(i + 1);
          this.schedulesInternal = a.concat(b);
        }
        this.notify();
      }
    });

    return callback;
  }

  fetchSchedules = () => {
    console.log('Update schedules...');
    this.Loading.next(true);
    setTimeout(() => {
      this.schedulesInternal = schedules;
      this.Loading.next(false);
      this.notify();
    }, 2500);
  };

  private getNewScheduleID = () => {
    const IDs = this.schedulesInternal.map((s) => s.ID);
    IDs.sort((s1, s2) => (s1 as number) - (s2 as number));
    return (IDs[IDs.length - 1] as number) + 1;
  };
}
