module.exports = [
  {
    "type": "heading",
    "defaultValue": "Sweeep Appearance Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background"
      },
      {
        "type": "color",
        "capabilities": ["COLOR"],
        "messageKey": "HourColor",
        "defaultValue": "0xFFFFFF",
        "label": "Hour Hand"
      },
      {
        "type": "color",
        "capabilities": ["COLOR"],
        "messageKey": "MinuteColor",
        "defaultValue": "0xFF5555",
        "label": "Minute Hand"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
