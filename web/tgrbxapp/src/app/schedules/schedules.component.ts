import { Component, ElementRef, Input, OnInit, ViewChild } from '@angular/core';
import { Location } from '@angular/common';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { EScheduleMode } from '../../lib/EScheduleMode';
import { FormControl } from '@angular/forms';
import { getTimestamp, timePickerToUnix } from '../../lib/DateConverter';
import { MatDialog, MatDialogRef } from '@angular/material/dialog';
import { ConfirmationDialogComponent } from '../common/confirmation-dialog/confirmation-dialog.component';
import { MatSnackBar, MatSnackBarRef } from '@angular/material/snack-bar';
import { HttpResponse } from '@angular/common/http';

@Component({
  selector: 'app-schedules',
  templateUrl: './schedules.component.html',
  styleUrls: ['./schedules.component.scss'],
})
export class SchedulesComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;
  public modeIDs = Object.keys(EScheduleMode)
    .filter((key) => !isNaN(Number(key)))
    .map((key: string) => Number(key));

  public getTimestamp = getTimestamp;
  public timePickerToUnix = timePickerToUnix;

  public schedulesListControl = new FormControl();

  @ViewChild('scheduleDayTimeInput', { static: false })
  dayTimesInput!: ElementRef;
  public dayTimesPickerOutput: string = '';

  @ViewChild('inpMaxTimesStart', { static: false })
  maxTimesStartInput!: ElementRef;
  public maxTimesStartPickerOutput: string = '';

  public schedules: Schedule[] = [];
  public FetchingSchedules: boolean = true;
  public selectedSchedule: Schedule | null = null;

  public toggledScheduleID: number | undefined = undefined;
  public saveOrDeletingSchedule = false;

  constructor(
    private location: Location,
    public scheduleService: ScheduleService,
    public dialog: MatDialog,
    private snackBar: MatSnackBar
  ) {}

  ngOnInit(): void {
    this.schedulesListControl.valueChanges.subscribe(
      this.onScheduleSelectionChanged
    );

    this.scheduleService.Schedules.subscribe((s: Schedule[] | null) => {
      if (!s) {
        return;
      }
      this.schedules = s;
      if (!this.selectedSchedule) {
        const selected = this.schedules.find((s) => s.Selected);
        if (selected) {
          this.schedulesListControl.setValue([selected]);
        }
      }
    });
    this.scheduleService.FetchingSchedules.subscribe((f) => {
      this.FetchingSchedules = f;
    });
  }

  onScheduleSelectionChanged = (selectedOptions: Schedule[]) => {
    const selectedSchedule = selectedOptions[0];
    this.selectedSchedule = selectedSchedule
      ? structuredClone(selectedSchedule)
      : null;
  };

  toggleScheduleActive(schedule: Schedule, active: boolean) {
    if (this.toggledScheduleID) {
      return;
    }
    //const updateSchedule = structuredClone(schedule);
    this.toggledScheduleID = schedule.ID;
    this.scheduleService
      .toggleScheduleActive(schedule, active)
      .subscribe((r: HttpResponse<unknown>) => {
        this.toggledScheduleID = undefined;
      });
  }

  onMaxTimesChanged = (newValue: number) => {
    if (this.selectedSchedule) {
      this.selectedSchedule.MaxTimes = newValue;
    }
  };

  btnAdd = () => {
    this.schedulesListControl.setValue([]);
    this.selectedSchedule = {
      Active: false,
      Selected: false,
      OnlyWhenEmpty: false,
      MaxTimes: 1,
      MaxTimesStartTime: 0,
    };
  };

  openDeleteDialog(
    enterAnimationDuration: string,
    exitAnimationDuration: string
  ): void {
    const header =
      'Futterplan "' + this.selectedSchedule?.Name + '" wirklich löschen?';

    const dialogRef = this.dialog.open(ConfirmationDialogComponent, {
      width: '250px',
      enterAnimationDuration,
      exitAnimationDuration,
      data: { header: header },
    });

    dialogRef.afterClosed().subscribe((result) => {
      if (result) {
        this.deleteSchedule();
      }
    });
  }

  openSnackBar(message: string) {
    this.snackBar.open(message, 'Ok', { duration: 2000 });
  }

  deleteSchedule = () => {
    if (!this.selectedSchedule) {
      return;
    }
    this.saveOrDeletingSchedule = true;
    this.scheduleService
      .deleteSchedule(this.selectedSchedule)
      .subscribe((r: HttpResponse<unknown>) => {
        if (r.status == 200) {
          this.openSnackBar('Gelöscht!');
          this.schedulesListControl.setValue([]);
        } else {
          this.openSnackBar('Fehler beim Löschen des Futterplans!');
        }
        this.saveOrDeletingSchedule = false;
      });
  };

  btnCancel = () => {
    this.schedulesListControl.setValue([]);
  };

  btnSave = () => {
    if (!this.selectedSchedule) {
      return;
    }

    if (!this.validateSchedule(this.selectedSchedule)) {
      this.openSnackBar('Bitte fülle alle Pflichtfelder aus!');
      return;
    }

    const callback = (r: HttpResponse<any>) => {
      if (r.status == 200) {
        this.schedulesListControl.setValue([]);
        this.openSnackBar('Gespeichert!');
      } else {
        this.openSnackBar('Fehler beim Speichern der Änderungen!');
      }
      this.saveOrDeletingSchedule = false;
    };

    this.saveOrDeletingSchedule = true;

    if (!!this.selectedSchedule.ID) {
      this.scheduleService
        .updateSchedule(this.selectedSchedule)
        .subscribe(callback);
    } else {
      this.scheduleService
        .addSchedule(this.selectedSchedule)
        .subscribe(callback);
    }
  };

  validateSchedule = (schedule: Schedule) => {
    let valid = false;
    console.log(schedule);
    switch (true) {
      case schedule.Mode == EScheduleMode.MaxTimes:
        valid =
          !!schedule.Name &&
          !!schedule.MaxTimes &&
          schedule.MaxTimesStartTime != undefined;
        break;
      case schedule.Mode == EScheduleMode.FixedDaytime:
        valid =
          !!schedule.Name &&
          !!schedule.Daytimes &&
          schedule.Daytimes.length > 0;
        break;
    }

    return valid;
  };

  onMaxTimeStartInput = (e: any) => {
    if (!this.selectedSchedule) {
      return;
    }

    const unix = timePickerToUnix(this.maxTimesStartPickerOutput);
    this.selectedSchedule.MaxTimesStartTime = unix;
  };

  onMaxTimeStartDisplayClick = (e: any) => {
    this.maxTimesStartInput.nativeElement.click();
  };

  btnAddDaytime = () => {
    this.dayTimesInput.nativeElement.click();
  };

  onDayTimeInput = (e: any) => {
    if (!this.selectedSchedule) {
      return;
    }

    if (!this.selectedSchedule.Daytimes) {
      this.selectedSchedule.Daytimes = [];
    }

    const newDaytimeString = this.dayTimesInput.nativeElement.value;
    const date = timePickerToUnix(newDaytimeString);
    if (!isNaN(date) && !this.selectedSchedule.Daytimes.includes(date)) {
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
