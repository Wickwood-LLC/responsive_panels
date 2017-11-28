(function ($) {
  // Update the window dimensions on each resize.
  $(window).on("resize", function () {
    Drupal.behaviors.responsive_panels.onResize();
  });

  Drupal.behaviors.responsive_panels = {
    breakpoint_panes: {},
    breakpoint_handler_exist: [],

    attach: function (context) {
      /**
       * Initializes breakpoint panels listeners and handling.
       */

      // If no breakpoints found, then these are not the droids you're looking for, move along.
      if (
        Drupal.settings.responsive_panels['breakpoints'] == 'undefined'
        || !enquire || !enquire.register
      ) {
        return;
      }
      var settings = Drupal.settings.responsive_panels;
      var breakpoints = settings['breakpoints'];

      var that = this;

      // Setup the toggle responsive handlers for use in enquire.js.
      // These are required or an unregister call will blow away handles that are still needed.
      for (var breakpoint in breakpoints) {
        var css = breakpoints[breakpoint]['css'];
        Drupal.settings.responsive_panels['breakpoints'][breakpoint]['toggle_handler'] = {
          match: function () {
            if($('.panels-ipe-editing').length < 1) {
              $('.hide-' + css).hide();
            }
          },
          unmatch: function () {
            if($('.panels-ipe-editing').length < 1) {
              $('.hide-' + css).show();
            }
          }
        };
      }

      // Check to see if an admin is using the IPE.
      $('#panels-ipe-customize-page').click(function (context) {
        that.checkForEditing();
      });

      // For each AJAX pane check if it should be loaded and register enquire match.
      $('.bp-ajax-pane', context).once('bp', function () {
        var element = $(this);

        // Kick the hide-* styles up the the .panel-pane to make sure any styles applied to the pane
        // do not still show even if the contents are just a placeholder.
        var parent_classes = element.parent().attr('class').split(/\s+/);
        var pane_ancestor = element.closest('.panel-pane');
        var ipe_ancestor = element.closest('.panels-ipe-portlet-wrapper');
        if (parent_classes.length) {
          for (var style in parent_classes) {
            element.removeClass(parent_classes[style]);
            pane_ancestor.addClass(parent_classes[style]);
            ipe_ancestor.addClass('ipe-' + parent_classes[style]);
          }
        }

        var breakpoints = that.identifyBreakpoints(element);
        var id = element.attr('id');
        $.each(breakpoints, function(index, bp) {
          if (!that.breakpoint_panes[bp]) {
            that.breakpoint_panes[bp] = []
          }
          that.breakpoint_panes[bp].push(id);
        })
      });

      var panes_to_fetch_now = [];
      $.each(this.breakpoint_panes, function(bp){
        if (that.breakpoint_handler_exist.indexOf(bp) < 0) {
          if (!matchMedia(bp).matches) {
            enquire.register(bp, {
              breakpoint: bp,
              match: function() {
                if (that.breakpoint_panes[this.breakpoint] && that.breakpoint_panes[this.breakpoint].length) {
                  that.fetch_panes(that.breakpoint_panes[this.breakpoint]);
                  // Clear the pane ids we processed.
                  that.breakpoint_panes[this.breakpoint] = [];
                }
              }
            });
            that.breakpoint_handler_exist.push(bp);
          }
          else {
            // Collect these pane ids to process now.
            panes_to_fetch_now.push.apply(panes_to_fetch_now, that.breakpoint_panes[bp]);

            that.breakpoint_panes[bp] = [];
          }
        }
      });
      if (panes_to_fetch_now.length) {
        this.fetch_panes(panes_to_fetch_now);
      }

      // Do a first manual update to catch the current window dimensions.
      this.onResize();
    },

    onResize: function () {
      /**
       * Updates the objects height/width and checks if reloading of the page is required.
       */
      var that = this;
      if (this.width && this.height) {
        this.checkForReload();
      }
      var $window = $(window);
      this.width = $window.width();
      this.height = $window.height()
    },

    checkForReload: function () {
      /**
       * If auto loading is enabled in the Breakpoint Panels configuration, then this
       * method will check if the page needs to be reloaded on a resize.
       * This is generally for development purposes.
       */

      var settings = Drupal.settings.responsive_panels;
      var breakpoints = settings['breakpoints'];

      if (!(settings['autoload'])) {
        return;
      }

      var $window = $(window);
      for (var breakpoint in breakpoints) {
        for (var key in breakpoints[breakpoint]) {
          // Skip any non-dimensional properties.
          if (key == 'bp' || key == 'css' || key == 'toggle_handler') {
            continue;
          }

          var value = breakpoints[breakpoint][key];

          // If the result changes, the condition has changed, so we need
          // to reload.
          var now = this.checkCondition(key, value, $window.width(), $window.height());
          var before = this.checkCondition(key, value, this.width, this.height);

          if (now !== before) {
            window.location.reload(true);

            // FF prevents reload in onRsize event, so we need to do it
            // in a timeout. See issue #1859058
            if ('mozilla' in $.browser) {
              setTimeout(function () {
                window.location.reload(true);
              }, 10);
            }
            return;
          }
        }
      }

    },

    checkCondition: function (condition, value, width, height) {
      /**
       * Used to check if a media query condition is met.
       */

      var flag = null;

      switch (condition) {
        case 'width':
          flag = width === value;
          break;

        case 'min-width':
          flag = width >= value;
          break;

        case 'max-width':
          flag = width <= value;
          break;

        case 'height':
          flag = height === value;
          break;

        case 'min-height':
          flag = height >= value;
          break;

        case 'max-height':
          flag = height <= value;
          break;

        case 'aspect-ratio':
          flag = width / height === value;
          break;

        case 'min-aspect-ratio':
          flag = width / height >= value;
          break;

        case 'max-aspect-ratio':
          flag = width / height <= value;
          break;

        default:
          break;
      }

      return flag;

    },

    identifyBreakpoints: function (element) {
      /**
       * Checks if a pane should be loaded given the current screen size.
       */

      var settings = Drupal.settings.responsive_panels;
      var breakpoints = settings['breakpoints'];

      var parent_el = element.parent();
      var bp_group = parent_el.attr('data-bp-group');
      var pane_breakpoints = [];
      for (var breakpoint in breakpoints) {
        // Determine whether breakpoint belongs to the group where pane is configured.
        // If not, then no need think about loading content via Ajax.
        if (typeof bp_group !== typeof undefined && bp_group !== false && breakpoints[breakpoint]['groups'].indexOf(bp_group) < 0) {
          continue;
        }
        if (
          !parent_el.hasClass('hide-' + breakpoints[breakpoint]['css'])
          || settings['loadhidden']
          || (settings['adminload'] && settings['isloggedin'])
        ) {
          pane_breakpoints.push(breakpoints[breakpoint]['bp']);
        }
      }
      return pane_breakpoints;
    },

    checkForEditing: function (x) {
      /**
       * Set up the breakpoint panels editing within IPE.
       */

      // Check if the IPE save button is there.
      x = (x) ? x : 0;
      var that = this;
      if ($('#panels-ipe-save').length < 1) {
        // Nope, wait more and try a few more times for good measure.
        x++;
        if (x < 10) {
          setTimeout(function () {
            that.checkForEditing(x);
          }, 500);
        }
        return;
      }

      var settings = Drupal.settings.responsive_panels;
      var breakpoints = settings['breakpoints'];

      // Setup the toggle responsive button.
      if ($('.toggleResponsive').length < 1) {
        $('#panels-ipe-edit-control-form div').prepend("<div class='toggleResponsive icon-large icon-eye-open'>Toggle Responsive</div>");
        $('.toggleResponsive').click(function () {
          if (!$(this).hasClass('active')) {

            for (var breakpoint_r in breakpoints) {
              if (enquire && enquire.register) {
                enquire.register(breakpoints[breakpoint_r]['bp'], breakpoints[breakpoint_r]['toggle_handler']);
              }
            }

            $(this).addClass('active icon-eye-close');
            $(this).removeClass('icon-eye-open');
            $('.panels-ipe-editing').addClass('hide-responsive');
          }
          else {
            for (var breakpoint_u in breakpoints) {
              if (enquire && enquire.register) {
                enquire.unregister(breakpoints[breakpoint_u]['bp'], breakpoints[breakpoint_u]['toggle_handler']);
              }
            }

            $(this).removeClass('active icon-eye-close');
            $(this).addClass('icon-eye-open');
            $('.panels-ipe-editing').removeClass('hide-responsive');
          }
        });
      }

    },

    fetch_panes: function (pane_ids) {
      var panes_submit_data = [];
      var base;
      $.each(pane_ids, function(index, id){
        var element = $('#' + id);
        // Does an AJAX request for the pane contents if it has not yet been loaded.
        // Need to check if element does exist since it might have replaced already.
        if (element.length && !element.hasClass('processed')) {
          var submit_data = {
            id: element.attr('id'),
            context: element.attr('data-context'),
            // 'data-query' attribute will be present if pane is view pane.
            // It is to pass original query params to view to generate correct results.
            query: $(element).attr('data-query')
          };
          base = element.attr('id');
          element.addClass('processed');
          panes_submit_data.push(submit_data);
        }
      });

      if (panes_submit_data.length) {
        var ajax_settings = {};
        ajax_settings.url = Drupal.settings.responsive_panels.url;
        ajax_settings.submit = {
          panes: panes_submit_data
        };
        var ajax = new Drupal.ajax(base, $('body'), ajax_settings);
        ajax.eventResponse($('body'), 'click');
      }
    }

  };

})(jQuery);
