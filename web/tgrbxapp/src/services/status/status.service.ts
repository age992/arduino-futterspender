import { Injectable } from '@angular/core';
import { IStatusService } from './status.service.interface';
import { BehaviorSubject, Subject, catchError, throwError } from 'rxjs';
import { MachineStatus } from 'src/models/MachineStatus';
import { HttpClient, HttpErrorResponse } from '@angular/common/http';
import { environment } from 'src/environments/environment';
import { FetchInterval } from 'src/models/FetchInterval';
import { WebsocketService } from '../websocket/websocket.service';
import { WebSocketData } from 'src/models/WebSocketData';

@Injectable({
  providedIn: 'root',
})
export class StatusService implements IStatusService {
  public MachineStatus = new BehaviorSubject<MachineStatus | null>(null);

  private machineStatusInternal: MachineStatus | null = null;

  public Connected = new BehaviorSubject(true);
  public Loading = new BehaviorSubject(false);
  public SetupMode = new BehaviorSubject(false);

  constructor(
    private http: HttpClient,
    private websocketService: WebsocketService
  ) {
    //this.setFetching(this.fetchIntervalNormal);
    websocketService.webSocketData.subscribe((d: WebSocketData | null) => {
      if (!!d) {
        this.machineStatusInternal = d.status;
        this.notify();
      }
    });
  }

  private notify = () => {
    this.MachineStatus.next(this.machineStatusInternal);
  };

  startFeed = () => {
    console.log('Start feeding...');
    //this.http.get(environment.apiUrl + '/food/open').subscribe();
  };

  stopFeed = () => {
    console.log('...feeding stopped.');
    //this.http.get(environment.apiUrl + '/food/close').subscribe();
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

  //--------------------------------------
  /*
  public MachineStatus = new BehaviorSubject<MachineStatus | null>(null);

  private machineStatusInternal: MachineStatus | null = null;

  public Connected = new BehaviorSubject(true);
  public Loading = new BehaviorSubject(false);
  public SetupMode = new BehaviorSubject(false);

  private lastUpdate = 0;
  private changeStatusDelay = 20000;
  private updateInterval = 10000;

  constructor() {
    this.fetchMachineStatus();
    setInterval(this.fetchMachineStatus, this.updateInterval);
  }

  setFetching(interval: FetchInterval): void {
    throw new Error('Method not implemented.');
  }

  private notify = () => {
    this.MachineStatus.next(this.machineStatusInternal);
  };

  public fetchMachineStatus = () => {
    console.log('Update status...');
    this.Loading.next(true);

    setTimeout(() => {
      const now = new Date().getTime();
      if (now - this.lastUpdate >= this.changeStatusDelay) {
        const newMachineStatus: MachineStatus = {
          ContainerLoad: Math.floor(Math.random() * 1000 * 100) / 100,
          PlateLoad: Math.floor(Math.random() * 10 * 100) / 100,
          MotorOperation: true,
          SDCardConnection: true,
          WiFiConnection: true,
        };
        this.lastUpdate = now;
        this.machineStatusInternal = newMachineStatus;
        this.notify();
      }

      this.Loading.next(false);
    }, 1000);
  };

  startFeed = () => {
    console.log('Start feeding...');
  };

  stopFeed = () => {
    console.log('...feeding stopped.');
  };*/
}
