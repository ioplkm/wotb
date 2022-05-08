#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <unistd.h>

#include <curl/curl.h>
#include <json-c/json.h>

typedef struct achievements {
  uint32_t markOfMastery;
  uint32_t markOfMasteryI;
  uint32_t markOfMasteryII;
  uint32_t markOfMasteryIII;
} achievements;

typedef struct stats {
  uint32_t damageDealt;
  uint32_t shots;
  uint32_t hits;
  uint32_t wins;
  uint32_t losses;
  uint32_t battles;
  uint32_t survivedBattles;
  uint32_t frags;
  uint32_t spots;
  uint32_t xp;
  achievements medals;
} stats;

const char* accountId = "126219745";
const char* appId = "74198832ec124e1cfe22490f35a7085f";

const char* timeUrl = "https://api.wotblitz.ru/wotb/account/info/?application_id=%s&account_id=%s&fields=last_battle_time";
const char* dataUrl = "https://api.wotblitz.ru/wotb/account/info/?application_id=%s&account_id=%s&fields=statistics.all.battles%%2Cstatistics.all.damage_dealt%%2Cstatistics.all.wins%%2Cstatistics.all.survived_battles%%2Cstatistics.all.hits%%2Cstatistics.all.shots%%2Cstatistics.all.losses%%2Cstatistics.all.frags%%2Cstatistics.all.spotted%%2Cstatistics.all.xp";
const char* achievementUrl = "https://api.wotblitz.ru/wotb/account/achievements/?application_id=%s&account_id=%s&fields=achievements";

time_t lastBattleTime, newTime;

stats initialStats;
stats lastStats;
stats battleStats;
stats sessionStats;
stats currentStats;

size_t timeParse(void* buffer, size_t size, size_t nmemb, time_t *pTime);
size_t dataParse(void* buffer, size_t size, size_t nmemb, stats *pStats);
size_t achievementParse(void* buffer, size_t size, size_t nmemb, achievements *pAchievements);

void calculateStatsDifference(stats *currentStats, stats *lastStats, stats *battleStats);

int main() {
  curl_global_init(CURL_GLOBAL_ALL);
  char *url = (char*)malloc(sizeof(char) * 999); 

  CURL *timeHandle = curl_easy_init();
  sprintf(url, timeUrl, appId, accountId);
  curl_easy_setopt(timeHandle, CURLOPT_URL, url);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEFUNCTION, timeParse);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEDATA, &lastBattleTime);
  curl_easy_perform(timeHandle);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEDATA, &newTime);

  CURL *dataHandle = curl_easy_init();
  sprintf(url, dataUrl, appId, accountId);
  curl_easy_setopt(dataHandle, CURLOPT_URL, url);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEFUNCTION, dataParse);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEDATA, &initialStats);
  curl_easy_perform(dataHandle);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEDATA, &currentStats);

  CURL *achievementHandle = curl_easy_init();
  sprintf(url, achievementUrl, appId, accountId);
  curl_easy_setopt(achievementHandle, CURLOPT_URL, url);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEFUNCTION, achievementParse);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEDATA, &initialStats.medals);
  curl_easy_perform(achievementHandle);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEDATA, &currentStats.medals);

  free(url);
  lastStats = initialStats;

  printf("\x1B[2J\x1B[H");
  printf(" num | damage | hitrate | winrate | survival | frags | spots |  xp  | M\n");
  for (;;) {
    curl_easy_perform(timeHandle);
    if (newTime != lastBattleTime) {
      lastBattleTime = newTime;
      curl_easy_perform(dataHandle);
      curl_easy_perform(achievementHandle);
      calculateStatsDifference(&currentStats, &lastStats, &battleStats);
      calculateStatsDifference(&currentStats, &initialStats, &sessionStats);
      printf("\x1B[2K\r");
      printf("%4i | %6i | %6.2f%% | %7s | %8s | %5d | %5d | %4d | %c\n", 
             sessionStats.battles,
             battleStats.damageDealt,
             (float)battleStats.hits / battleStats.shots * 100,
             battleStats.wins ? "win" : battleStats.losses ? "lose" : "draw",
             battleStats.survivedBattles ? "survived" : "died",
             battleStats.frags,
             battleStats.spots,
             (int)roundf(battleStats.xp / 1.5f),
             battleStats.medals.markOfMastery ? 'M' :
             battleStats.medals.markOfMasteryI ? '1' :
             battleStats.medals.markOfMasteryII ? '2' :
             battleStats.medals.markOfMasteryIII ? '3' : '-');
      printf(" avg |%7.1f | %6.2f%% | %6.2f%% | %7.2f%% | %5.2f | %5.2f | %4d | %c",
             (float)sessionStats.damageDealt / sessionStats.battles,
             (float)sessionStats.hits / sessionStats.shots * 100,
             (float)sessionStats.wins / sessionStats.battles * 100,
             (float)sessionStats.survivedBattles / sessionStats.battles * 100,
             (float)sessionStats.frags / sessionStats.battles,
             (float)sessionStats.spots / sessionStats.battles,
             (int)roundf(sessionStats.xp / sessionStats.battles / 1.5f),
             sessionStats.medals.markOfMastery ? 'M' :
             sessionStats.medals.markOfMasteryI ? '1' :
             sessionStats.medals.markOfMasteryII ? '2' :
             sessionStats.medals.markOfMasteryIII ? '3' : '-');
      fflush(stdout);
      lastStats = currentStats;
    }
    sleep(1);
  }
  curl_global_cleanup();
  return 0;
}

size_t timeParse(void *buffer, size_t size, size_t nmemb, time_t *pTime) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *last_battle_time;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "last_battle_time", &last_battle_time);
  *pTime = json_object_get_int(last_battle_time);
  return size * nmemb;
}

size_t dataParse(void *buffer, size_t size, size_t nmemb, stats *pStats) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *statistics, *all;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "statistics", &statistics);
  json_object_object_get_ex(statistics, "all", &all);
  json_object *damage_dealt, *shots, *hits, *battles, *survived_battles, *wins, *losses, *frags, *spotted, *xp;
  json_object_object_get_ex(all, "damage_dealt", &damage_dealt);
  json_object_object_get_ex(all, "shots", &shots);
  json_object_object_get_ex(all, "hits", &hits);
  json_object_object_get_ex(all, "wins", &wins);
  json_object_object_get_ex(all, "battles", &battles);
  json_object_object_get_ex(all, "survived_battles", &survived_battles);
  json_object_object_get_ex(all, "losses", &losses);
  json_object_object_get_ex(all, "frags", &frags);
  json_object_object_get_ex(all, "spotted", &spotted);
  json_object_object_get_ex(all, "xp", &xp);
  pStats->damageDealt = json_object_get_int(damage_dealt);
  pStats->hits = json_object_get_int(hits);
  pStats->shots = json_object_get_int(shots);
  pStats->wins = json_object_get_int(wins);
  pStats->losses = json_object_get_int(losses);
  pStats->battles = json_object_get_int(battles);
  pStats->survivedBattles = json_object_get_int(survived_battles);
  pStats->frags = json_object_get_int(frags);
  pStats->spots = json_object_get_int(spotted);
  pStats->xp = json_object_get_int(xp);
  return size * nmemb;
}

size_t achievementParse(void *buffer, size_t size, size_t nmemb, achievements *pAchievements) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *achievements;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "achievements", &achievements);
  json_object *markOfMastery, *markOfMasteryI, *markOfMasteryII, *markOfMasteryIII;
  json_object_object_get_ex(achievements, "markOfMastery", &markOfMastery);
  json_object_object_get_ex(achievements, "markOfMasteryI", &markOfMasteryI);
  json_object_object_get_ex(achievements, "markOfMasteryII", &markOfMasteryII);
  json_object_object_get_ex(achievements, "markOfMasteryIII", &markOfMasteryIII);
  pAchievements->markOfMastery = json_object_get_int(markOfMastery);
  pAchievements->markOfMasteryI = json_object_get_int(markOfMasteryI);
  pAchievements->markOfMasteryII = json_object_get_int(markOfMasteryII);
  pAchievements->markOfMasteryIII = json_object_get_int(markOfMasteryIII);
  return size * nmemb;
}

void calculateStatsDifference(stats *currentStats, stats *lastStats, stats *battleStats) {
  battleStats->damageDealt = currentStats->damageDealt - lastStats->damageDealt;
  battleStats->hits = currentStats->hits - lastStats->hits;
  battleStats->shots = currentStats->shots - lastStats->shots;
  battleStats->wins = currentStats->wins - lastStats->wins;
  battleStats->losses = currentStats->losses - lastStats->losses;
  battleStats->battles = currentStats->battles - lastStats->battles;
  battleStats->survivedBattles = currentStats->survivedBattles - lastStats->survivedBattles;
  battleStats->frags = currentStats->frags - lastStats->frags;
  battleStats->spots = currentStats->spots - lastStats->spots;
  battleStats->xp = currentStats->xp - lastStats->xp;
  battleStats->medals.markOfMastery = currentStats->medals.markOfMastery - lastStats->medals.markOfMastery;
  battleStats->medals.markOfMasteryI = currentStats->medals.markOfMasteryI - lastStats->medals.markOfMasteryI;
  battleStats->medals.markOfMasteryII = currentStats->medals.markOfMasteryII - lastStats->medals.markOfMasteryII;
  battleStats->medals.markOfMasteryIII = currentStats->medals.markOfMasteryIII - lastStats->medals.markOfMasteryIII;
}
