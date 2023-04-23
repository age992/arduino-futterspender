import { Component, ElementRef, OnInit, ViewChild } from '@angular/core';
import { Location } from '@angular/common';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { EScheduleMode } from '../lib/EScheduleMode';
import { FormControl } from '@angular/forms';
import { getTimestamp, timePickerToUnix } from '../lib/DateConverter';

@Component({
  selector: 'app-schedules',
  templateUrl: './schedules.component.html',
  styleUrls: ['./schedules.component.scss'],
})
export class SchedulesComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;
  public getTimestamp = getTimestamp;
  public timePickerToUnix = timePickerToUnix;

  public modeIDs = Object.keys(EScheduleMode)
    .filter((key) => !isNaN(Number(key)))
    .map((key: string) => Number(key));
  public dayTimes: number[] = [1291410984924091];

  public schedulesListControl = new FormControl();
  @ViewChild('scheduleTimeInput', { static: false }) timeInput!: ElementRef;
  public myValue: string = '';

  public schedules: Schedule[] = [];
  public Loading: boolean = true;
  public selectedSchedule: Schedule | null = null;

  constructor(
    private location: Location,
    public scheduleService: ScheduleService
  ) {}

  ngOnInit(): void {
    this.schedulesListControl.valueChanges.subscribe(
      this.onScheduleSelectionChanged
    );

    this.scheduleService.Schedules.subscribe((s) => {
      this.schedules = s;
      if (!this.selectedSchedule) {
        const selected = this.schedules.find((s) => s.Selected);
        if (selected) {
          this.schedulesListControl.setValue([selected]);
        }
      }
    });
    this.scheduleService.Loading.subscribe((l) => {
      this.Loading = l;
    });
  }

  onScheduleSelectionChanged = (selectedOptions: Schedule[]) => {
    const selectedSchedule = selectedOptions[0];
    this.selectedSchedule = selectedSchedule
      ? structuredClone(selectedSchedule)
      : null;
  };

  onMaxTimesChanged = (newValue: number) => {
    if (this.selectedSchedule) {
      this.selectedSchedule.MaxTimes = newValue;
    }
  };

  btnAdd = () => {
    console.log('new');
    this.schedulesListControl.setValue([]);
    this.selectedSchedule = {};
  };

  btnCancel = () => {
    this.schedulesListControl.setValue([]);
  };

  btnSave = () => {
    if (!this.selectedSchedule) {
      return;
    }
    console.log('Save schedule...');

    this.scheduleService
      .upsertSchedule(this.selectedSchedule)
      .subscribe((r) => {
        if (r.status == 200) {
          this.schedulesListControl.setValue([]);
          console.log('Success!');
        } else {
          console.log('Error warning!');
        }
      });
  };

  btnAddDaytime = () => {
    this.timeInput.nativeElement.click();
  };

  onTimeInput = (e: any) => {
    if (!this.selectedSchedule) {
      return;
    }

    if (!this.selectedSchedule.Daytimes) {
      this.selectedSchedule.Daytimes = [];
    }

    const newDaytimeString = this.timeInput.nativeElement.value;
    const date = timePickerToUnix(newDaytimeString);
    if (!isNaN(date)) {
      this.selectedSchedule.Daytimes.push(date);
      this.selectedSchedule.Daytimes.sort();
    }
  };

  removeDaytime = (daytime: number) => {
    if (!this.selectedSchedule?.Daytimes) {
      return;
    }

    const index = this.selectedSchedule.Daytimes.indexOf(daytime);
    if (index != -1) {
      const a = this.selectedSchedule.Daytimes.slice(0, index);
      const b = this.selectedSchedule.Daytimes.slice(index + 1);
      this.selectedSchedule.Daytimes = a.concat(b);
    }
  };

  navBack() {
    this.location.back();
  }
}
