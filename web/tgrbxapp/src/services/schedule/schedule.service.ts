import { Injectable, OnInit } from '@angular/core';
import { IScheduleService } from './schedule.service.interface';
import { Schedule } from 'src/models/Schedule';
import { BehaviorSubject, Observable, ReplaySubject } from 'rxjs';
import { HttpClient, HttpResponse } from '@angular/common/http';
import { environment } from 'src/environments/environment';

@Injectable({
  providedIn: 'root',
})
export class ScheduleService implements IScheduleService {
  Schedules = new BehaviorSubject<Schedule[] | null>(null);
  FetchingSchedules: BehaviorSubject<boolean> = new BehaviorSubject(false);

  private schedulesInternal: Schedule[] = [];

  private notify = () => {
    this.Schedules.next(this.schedulesInternal);
  };

  constructor(private http: HttpClient) {
    this.fetchSchedules();
  }

  toggleScheduleActive(
    schedule: Schedule,
    active: boolean
  ): Observable<HttpResponse<any>> {
    console.log('Toggle schedule ', schedule.ID, active);
    const obs = this.http.get<HttpResponse<any>>(
      environment.apiUrl +
        '/schedule/activate?id=' +
        schedule.ID +
        '&active=' +
        (active ? 'true' : 'false'),
      { observe: 'response' }
    );
    obs.subscribe((response: HttpResponse<any>) => {
      if (response.status == 200) {
        const lastSelected = this.schedulesInternal.find((s) => s.Selected);
        if (lastSelected && lastSelected.ID != schedule.ID) {
          lastSelected.Selected = false;
          lastSelected.Active = false;
        }

        schedule.Selected = true;
        schedule.Active = active;
        this.notify();
      }
    });
    return obs;
  }

  addSchedule(schedule: Schedule): Observable<HttpResponse<number>> {
    console.log('Add schedule...');
    const obs = this.http.post<HttpResponse<number>>(
      environment.apiUrl + '/schedule',
      schedule
    );
    obs.subscribe((response: HttpResponse<number>) => {
      schedule.ID = response.body as number;
      this.schedulesInternal.push(schedule);
      this.notify();
    });
    return obs;
  }

  updateSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    console.log('Update schedule...');
    const obs = this.http.put<HttpResponse<unknown>>(
      environment.apiUrl + '/schedule',
      schedule,
      { observe: 'response' }
    );
    obs.subscribe((response: HttpResponse<unknown>) => {
      if (response.status == 200) {
        let oldSchedule = this.schedulesInternal.filter(
          (_) => _.ID == schedule.ID
        )[0];
        oldSchedule = schedule;
      }
      this.notify();
    });
    return obs;
  }

  deleteSchedule(schedule: Schedule): Observable<HttpResponse<unknown>> {
    const obs = this.http.delete<HttpResponse<any>>(
      environment.apiUrl + '/schedule?' + schedule.ID
    );
    obs.subscribe((response: HttpResponse<any>) => {
      if (response.status == 200) {
        let i = this.schedulesInternal.findIndex((s) => s.ID == schedule.ID);
        if (i != -1) {
          const a = this.schedulesInternal.slice(0, i);
          const b = this.schedulesInternal.slice(i + 1);
          this.schedulesInternal = a.concat(b);
        }
      }
      this.notify();
    });
    return obs;
  }

  fetchSchedules = () => {
    console.log('FetchSchedules...');
    this.http
      .get<ScheduleFetchResponse>(environment.apiUrl + '/schedule', {
        observe: 'response',
      })
      .subscribe((response) => {
        if (response.status == 200) {
          this.schedulesInternal = response.body?.schedules || [];
          this.notify();
        }
      });
  };
}

export interface ScheduleFetchResponse {
  schedules: Schedule[];
}
