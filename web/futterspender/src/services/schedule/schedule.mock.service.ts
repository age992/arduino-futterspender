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
  Schedules = new BehaviorSubject<Schedule[] | null>(null);
  FetchingSchedules: BehaviorSubject<boolean> = new BehaviorSubject(false);

  private schedulesInternal: Schedule[] = [];

  constructor() {
    this.fetchSchedules();
  }

  private notify = () => {
    this.Schedules.next(this.schedulesInternal);
  };

  upsertSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    const clonedSchedule = structuredClone(schedule);
    if (!clonedSchedule.ID) {
      clonedSchedule.ID = this.getNewScheduleID();
    }

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
        let i = this.schedulesInternal.findIndex(
          (s) => s.ID == clonedSchedule.ID
        );
        if (i != -1) {
          this.schedulesInternal[i] = clonedSchedule;
        } else {
          this.schedulesInternal.push(clonedSchedule);
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
    this.FetchingSchedules.next(true);
    setTimeout(() => {
      this.schedulesInternal = schedules;
      this.FetchingSchedules.next(false);
      this.notify();
    }, 2500);
  };

  private getNewScheduleID = () => {
    const IDs = this.schedulesInternal.map((s) => s.ID);
    IDs.sort();
    let lastID = IDs[IDs.length - 1];
    return !lastID ? 1 : lastID + 1;
  };
}
