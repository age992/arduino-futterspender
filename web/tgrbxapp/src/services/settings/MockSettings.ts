import { ELanguage } from 'src/lib/ELanguage';
import { ETheme } from 'src/lib/ETheme';
import { Settings } from 'src/models/Settings';

export const settings: Settings = {
  Language: ELanguage.German,
  Theme: ETheme.Boring,
  Notifications: {
    ContainerEmpty: { Active: false, Email: false, Phone: false },
    DidNotEatInADay: { Active: false, Email: false, Phone: false },
  },
};
