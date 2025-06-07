var DEVMODE = window.location.protocol === 'file:';

const URL_APPCONFIG = "/!wificlock";

const DEBUG_CONFIG = {
  "time_offset": "5",
  "resync_time": "420",
};

const CONFIG_UPDATE_DELAY = 2000;
const CONFIG_DEFAULT_TIME_OFFSET = '0';
const CONFIG_DEFAULT_RESYNC_TIME = '360'

var cur_config = {};
var new_config = {};

var config_update_timer = null;

function config_received(config) {
  new_config = structuredClone(cur_config = config || {});
  refresh_config_ui();
}

function refresh_config_ui() {
  $("#time-offset").val(new_config.time_offset || CONFIG_DEFAULT_TIME_OFFSET);
  $("#resync-time").val(new_config.resync_time || CONFIG_DEFAULT_RESYNC_TIME);
}

function config_update_sync(failure_cnt = 0) {
  if (cur_config.time_offset !== new_config.time_offset ||
    cur_config.resync_time !== new_config.resync_time) {
    console.log('Updating config...', new_config);

    if (DEVMODE) {
      cur_config = structuredClone(new_config);
      setTimeout(config_update_sync, 100);
      return;
    }

    $.ajax({
      method: 'PUT',
      url: URL_APPCONFIG,
      data: JSON.stringify(new_config),
      contentType: 'application/json',
    }).done(function () {
      cur_config = structuredClone(new_config);
    }).fail(function (jqXHR, textStatus) {
      var resp_text = (typeof jqXHR.responseText !== 'undefined') ? jqXHR.responseText : textStatus;
      console.log("Failed to update config:", resp_text);
      ++failure_cnt;
      // TODO: Instead of forcibly losing data, prompt for retry or abort.
      new_config = structuredClone(cur_config);
    }).always(function () {
      setTimeout(config_update_sync, 0, failure_cnt);
    });
    return;
  }

  console.log("Config sync complete");
  $("fieldset.modifiable").removeClass("syncing");
  if (failure_cnt > 0) refresh_config_ui();
}

function config_update_probe() {
  if (config_update_timer) {
    clearTimeout(config_update_timer);
    config_update_timer = null;
  }

  if (_.isEqual(new_config, cur_config)) return;

  config_update_timer = setTimeout(function () {
    config_update_timer = null;
    $("fieldset.modifiable").addClass("syncing");
    console.log("Config sync start");
    config_update_sync(0);
  }, CONFIG_UPDATE_DELAY);
}

function print_time(time_mins) {
  const hours = Math.floor(time_mins / 60);
  const mins = time_mins % 60;
  return `${hours.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}`;
}

function offset_select_update() {
  new_config.time_offset = $("#time-offset").val();
  config_update_probe();
}

function resync_select_update() {
  new_config.resync_time = $("#resync-time").val();
  config_update_probe();
}

$(function () {
  $('#redir-ntp-tz').on('click', function () {
    window.top.location.href = "index.html#sys-mgmt";
  });

  const offset_select = $("#time-offset");
  for (var i = -30; i <= 30; i++) {
    offset_select.append(`<option value="${i}">${i}</option>`);
  }
  offset_select.val(undefined);
  offset_select.on('change', offset_select_update);

  const resync_select = $("#resync-time");
  for (var i = 12; i < 36; i++) {
    if (i >= 24 && i < 30) continue;
    const mins = (i % 24) * 60;
    resync_select.append(`<option value="${mins}">${print_time(mins)}</option>`);
  }
  resync_select.val(undefined);
  resync_select.on('change', resync_select_update);

  if (!DEVMODE) {
    $("fieldset.modifiable").removeClass("syncing");
    probe_url_for(URL_APPCONFIG, config_received, function (text) {
      block_prompt(`<p><center>WiFi Clock unavailable:<br>${text}</center>`);
    });
  } else {
    setTimeout(function () {
      config_received(structuredClone(DEBUG_CONFIG));
      setTimeout(function () {
        $("fieldset.modifiable").removeClass("syncing");
      }, 500);
    }, 200);
  }
});