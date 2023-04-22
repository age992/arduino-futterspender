import { Component, OnInit } from '@angular/core';
import { RouterOutlet } from '@angular/router';

import { slider } from '../animations/Slider';
import { StatusService } from 'src/services/status/status.service';
import { MachineStatus } from 'src/models/MachineStatus';

@Component({
  selector: 'app-root',
  animations: [slider],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss'],
})
export class AppComponent implements OnInit {
  public title = 'futterspender';
  public MachineStatus: MachineStatus | null = null;
  public Loading: boolean = false;
  public Connected: boolean = true;

  constructor(public statusService: StatusService) {}

  ngOnInit(): void {
    this.statusService.MachineStatus.subscribe((m) => {
      this.MachineStatus = m;
    });
    this.statusService.Loading.subscribe((l) => {
      this.Loading = l;
    });
    this.statusService.Connected.subscribe((c) => {
      this.Connected = c;
    });
  }

  getRouteAnimationData(outlet: RouterOutlet) {
    return outlet?.activatedRouteData['animation'];
  }

  btnFeed = (feed: boolean) => {
    if (feed) {
      this.statusService.startFeed();
    } else {
      this.statusService.stopFeed();
    }
  };
}
