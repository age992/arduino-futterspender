import { Injectable } from '@angular/core';
import { IStatusService } from './status.service.interface';
import { BehaviorSubject, Subject, catchError, throwError } from 'rxjs';
import { MachineStatus } from 'src/models/MachineStatus';
import { HttpClient, HttpErrorResponse } from '@angular/common/http';
import { environment } from 'src/environments/environment';
import { FetchInterval } from 'src/models/FetchInterval';

@Injectable({
  providedIn: 'root',
})
export class StatusService implements IStatusService {
  public MachineStatus = new BehaviorSubject<MachineStatus | null>(null);

  private apiBase = 'http://esp32.local/api';

  private machineStatusInternal: MachineStatus | null = null;

  public Connected = new BehaviorSubject(true);
  public Loading = new BehaviorSubject(false);
  public SetupMode = new BehaviorSubject(false);

  //in seconds
  private readonly fetchIntervalNormal: FetchInterval = {
    Active: false,
    IntervalSeconds: 4,
  };
  private readonly fetchIntervalFast: FetchInterval = {
    Active: false,
    IntervalSeconds: 0.3,
  };
  private readonly fetchFastChangeThreshold = 1; //change in gram per second
  private readonly fetchSlowGracePeriod = 6; //if no changes occur for this period of time, fetching will slow down
  private graceTimer: NodeJS.Timeout | null = null;

  constructor(private http: HttpClient) {
    this.setFetching(this.fetchIntervalNormal);
  }

  private notify = () => {
    this.MachineStatus.next(this.machineStatusInternal);
  };

  setFetching = (interval: FetchInterval) => {
    if (interval.Active) {
      return;
    }

    let activeInterval: FetchInterval | null = null;

    if (this.fetchIntervalNormal.Active) {
      activeInterval = this.fetchIntervalNormal;
    } else if (this.fetchIntervalFast.Active) {
      activeInterval = this.fetchIntervalFast;
    }

    if (!!activeInterval?.Timer) {
      clearInterval(activeInterval.Timer);
      activeInterval.Active = false;
    }

    interval.Timer = setInterval(
      this.fetchMachineStatus,
      interval.IntervalSeconds * 1000
    );

    interval.Active = true;
    this.fetchMachineStatus();
  };

  adjustInterval = (newStatus: MachineStatus) => {
    if (!this.machineStatusInternal) {
      return;
    }

    const d_container =
      newStatus.ContainerLoad - this.machineStatusInternal.ContainerLoad;
    const d_plate = newStatus.PlateLoad - this.machineStatusInternal.PlateLoad;

    if (
      Math.abs(d_container) > this.fetchFastChangeThreshold ||
      Math.abs(d_plate) > this.fetchFastChangeThreshold
    ) {
      this.setFetching(this.fetchIntervalFast);
      if (!!this.graceTimer) {
        clearTimeout(this.graceTimer);
        this.graceTimer = null;
      }
    } else if (this.fetchIntervalFast.Active) {
      if (!this.graceTimer) {
        this.graceTimer = setTimeout(
          () => this.setFetching(this.fetchIntervalNormal),
          this.fetchSlowGracePeriod * 1000
        );
      }
    }
  };

  fetchMachineStatus = () => {
    this.Loading.next(true);

    this.http
      .get<MachineStatus>(environment.apiUrl + '/status')
      .pipe(catchError((err) => this.handleError(err)))
      .subscribe((status) => {
        this.adjustInterval(status);
        this.machineStatusInternal = status;
        this.notify();
        this.Loading.next(false);
        this.Connected.next(true);
      });
  };

  startFeed = () => {
    console.log('Start feeding...');
    this.http.get(this.apiBase + '/container?open=true');
  };

  stopFeed = () => {
    console.log('...feeding stopped.');
    this.http.get(this.apiBase + '/container?open=false');
  };

  private handleError(error: HttpErrorResponse) {
    if (error.status === 0) {
      console.error('An error occurred:', error.error);
      if (error.error instanceof ProgressEvent) {
        this.Connected.next(false);
      }
    } else {
      console.error(
        `Backend returned code ${error.status}, body was: `,
        error.error
      );
    }
    return throwError(
      () => new Error('Something bad happened; please try again later.')
    );
  }
}
