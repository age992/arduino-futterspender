import { Component } from '@angular/core';
import { Location } from '@angular/common';
import { Schedule } from 'src/models/Schedule';

@Component({
  selector: 'app-schedules',
  templateUrl: './schedules.component.html',
  styleUrls: ['./schedules.component.scss'],
})
export class SchedulesComponent {
  public schedules!: Schedule[];

  constructor(private location: Location) {
    this.schedules = [];
  }

  navBack() {
    this.location.back();
  }
}
