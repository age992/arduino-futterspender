import { Component, OnInit } from '@angular/core';
import { Location } from '@angular/common';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';

@Component({
  selector: 'app-schedules',
  templateUrl: './schedules.component.html',
  styleUrls: ['./schedules.component.scss'],
})
export class SchedulesComponent implements OnInit {
  public schedules: Schedule[] = [];
  public selectedSchedule: Schedule | null | undefined = null;
  public newSchedule: Schedule | null | undefined = null;
  public Loading: boolean = true;

  constructor(
    private location: Location,
    public scheduleService: ScheduleService
  ) {}

  ngOnInit(): void {
    this.scheduleService.Schedules.subscribe((s) => {
      this.schedules = s;
    });
    /*const selected = this.schedules.find((s) => s.Selected);
    if (selected) {
      this.selectedSchedule = selected;
    }*/
    this.scheduleService.Loading.subscribe((l) => {
      this.Loading = l;
    });
  }

  onScheduleSelectionChange = (e: any) => {
    const newSelectedID = e.options[0].value;
    const newSelected = this.schedules.find((s) => s.ID == newSelectedID);
    this.selectedSchedule = newSelected;
  };

  getScheduleFormID = () => {
    if (this.selectedSchedule) {
      return this.selectedSchedule.ID;
    }

    const IDs = this.schedules.map((s) => s.ID);
    IDs.sort((s1, s2) => (s1 as number) - (s2 as number));
    return (IDs[IDs.length - 1] as number) + 1;
  };

  btnAdd = () => {};

  navBack() {
    this.location.back();
  }
}
