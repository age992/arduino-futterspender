import { Component, OnInit } from '@angular/core';
import { MachineStatus } from 'src/models/MachineStatus';
import { StatusService } from 'src/services/status/status.service';

@Component({
  selector: 'dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
})
export class DashboardComponent implements OnInit {
  public MachineStatus: MachineStatus | null = null;
  public Loading: boolean = false;
  public Connected: boolean = true;

  constructor(private statusService: StatusService) {}

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
}
