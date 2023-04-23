import { Injectable } from '@angular/core';
import { IStatusService } from './status.service.interface';
import { MachineStatus } from 'src/models/MachineStatus';
import { BehaviorSubject, ReplaySubject, Subject } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class StatusMockService implements IStatusService {
  public MachineStatus: ReplaySubject<MachineStatus | null> =
    new ReplaySubject();

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
        this.MachineStatus.next(newMachineStatus);
      }

      this.Loading.next(false);
    }, 1000);
  };

  startFeed = () => {
    console.log('Start feeding...');
  };

  stopFeed = () => {
    console.log('...feeding stopped.');
  };
}
