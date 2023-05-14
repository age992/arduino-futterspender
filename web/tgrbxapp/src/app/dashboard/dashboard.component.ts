import { Component, OnInit } from '@angular/core';
import { MachineStatus } from 'src/models/MachineStatus';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { StatusService } from 'src/services/status/status.service';
import { EScheduleMode } from 'src/lib/EScheduleMode';
import { getTimestamp } from 'src/lib/DateConverter';
import { SettingsService } from 'src/services/settings/settings.service';
import { Settings } from 'src/models/Settings';
import { HistoryService } from 'src/services/history/history.service';
import { HistoryData } from 'src/models/WebSocketData';
import { EventType } from 'src/models/Event';

@Component({
  selector: 'dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
})
export class DashboardComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;
  public getTimestamp = getTimestamp;

  public MachineStatus: MachineStatus | null = null;
  public LoadingStatus: boolean = false;
  public Connected: boolean = true;

  public settings: Settings | null = null;
  public FetchingSettings = false;

  public currentSchedule: Schedule | null = null;
  public nextFeedingTime: number | null = null;
  public FetchingSchedule: boolean = false;
  public UpdatingActivity: boolean = false;

  public History: HistoryData = {
    schedules: [],
    events: [],
    scaleData: [],
  };

  constructor(
    public statusService: StatusService,
    private scheduleService: ScheduleService,
    private settingsService: SettingsService,
    private historyService: HistoryService
  ) {}

  ngOnInit(): void {
    this.statusService.MachineStatus.subscribe((m: MachineStatus | null) => {
      this.MachineStatus = m;
    });
    this.statusService.Loading.subscribe((l) => {
      this.LoadingStatus = l;
    });
    this.statusService.Connected.subscribe((c) => {
      this.Connected = c;
    });

    this.settingsService.Settings.subscribe((s) => (this.settings = s));
    this.settingsService.FetchingSettings.subscribe(
      (f) => (this.FetchingSettings = f)
    );

    this.scheduleService.Schedules.subscribe((s) => {
      if (!s || s.length == 0) {
        return;
      }

      for (let schedule of s) {
        if (schedule.Selected) {
          this.currentSchedule = schedule;
          if (schedule.Daytimes) {
            this.updateNextFeed(schedule.Daytimes);
          }
          break;
        }
      }
    });

    this.scheduleService.FetchingSchedules.subscribe((l) => {
      this.FetchingSchedule = l;
    });

    this.historyService.HistoryData.subscribe((h) => {
      if (h.events && h.events.length > 0) {
        this.History.events.push(...h.events);
        if (
          h.events.findIndex(
            (e) =>
              e.Type == EventType.Feed ||
              e.Type == EventType.MissedFeed ||
              e.Type == EventType.SkippedFeed
          ) != -1 &&
          !!this.currentSchedule?.Daytimes
        ) {
          this.updateNextFeed(this.currentSchedule.Daytimes);
        }
      }
      if (h.scaleData && h.scaleData.length > 0) {
        this.History.scaleData.push(...h.scaleData);
      }
      if (h.schedules && h.schedules.length > 0) {
        this.History.schedules.push(...h.schedules);
      }
    });
  }

  updateNextFeed = (daytimes: number[]) => {
    daytimes.sort();
    const now = new Date();
    let daytimeNow = new Date(0);
    daytimeNow.setUTCHours(now.getUTCHours());
    daytimeNow.setUTCMinutes(now.getUTCMinutes());
    daytimeNow.setUTCSeconds(now.getUTCSeconds());
    const daytimeNowSeconds = daytimeNow.getTime() / 1000;
    let index = daytimes.findIndex((d) => d / 1000 > daytimeNowSeconds);
    const nextTime = index > -1 ? daytimes[index] : daytimes[0];
    this.nextFeedingTime = nextTime;
  };

  toggleScheduleActive = (active: boolean) => {
    /*const toggledSchedule: Schedule = {
      ...this.currentSchedule!,
      Active: active,
    };*/
    this.UpdatingActivity = true;
    this.scheduleService
      .toggleScheduleActive(this.currentSchedule!, active)
      .subscribe((s) => {
        this.UpdatingActivity = false;
      });
  };

  protected get containerLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.ContainerLoad / 1000) * 100;
  }

  protected get plateLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.PlateLoad / 100) * 100;
    // return this.MachineStatus!.PlateLoad / this.settings!.PlateFilling as number;
  }
}
