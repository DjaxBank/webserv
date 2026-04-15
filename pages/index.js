/**
 * index.js - Main JavaScript functionality
 */

// Utility function to log with timestamp
const log = (...args) => {
  console.log(`[${new Date().toLocaleTimeString()}]`, ...args);
};

// Initialize on DOM load
document.addEventListener('DOMContentLoaded', () => {
  log('Page loaded and initialized');
  initializeEventListeners();
  setupPageInteractions();
});

/**
 * Set up all event listeners
 */
function initializeEventListeners() {
  // Add click event to all buttons
  const buttons = document.querySelectorAll('button');
  buttons.forEach((button) => {
    button.addEventListener('click', handleButtonClick);
  });

  // Add input validation listeners
  const inputs = document.querySelectorAll('input');
  inputs.forEach((input) => {
    input.addEventListener('blur', validateInput);
    input.addEventListener('focus', clearInputError);
  });
}

/**
 * Handle button clicks with feedback
 * @param {Event} event - Click event
 */
function handleButtonClick(event) {
  const button = event.target;
  log('Button clicked:', button.textContent);

  // Add visual feedback
  button.style.transform = 'scale(0.95)';
  setTimeout(() => {
    button.style.transform = '';
  }, 150);
}

/**
 * Validate input fields
 * @param {Event} event - Blur event
 */
function validateInput(event) {
  const input = event.target;
  const value = input.value.trim();

  if (input.type === 'email' && value) {
    const isValid = /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value);
    if (!isValid) {
      addInputError(input, 'Please enter a valid email');
      return;
    }
  }

  if (input.required && !value) {
    addInputError(input, 'This field is required');
    return;
  }

  clearInputError(input);
}

/**
 * Add error styling to input
 * @param {HTMLElement} input - Input element
 * @param {string} message - Error message
 */
function addInputError(input, message) {
  input.style.borderColor = '#e74c3c';
  input.style.boxShadow = '0 0 0 3px rgba(231, 76, 60, 0.1)';

  // Create or update error message
  let errorDisplay = input.nextElementSibling;
  if (!errorDisplay || !errorDisplay.classList.contains('error-message')) {
    errorDisplay = document.createElement('div');
    errorDisplay.className = 'error-message';
    input.parentNode.insertBefore(errorDisplay, input.nextSibling);
  }
  errorDisplay.textContent = message;
  errorDisplay.style.color = '#e74c3c';
  errorDisplay.style.fontSize = '0.875rem';
  errorDisplay.style.marginTop = '-10px';
  errorDisplay.style.marginBottom = '10px';
}

/**
 * Clear error styling from input
 * @param {Event|HTMLElement} eventOrInput - Event or input element
 */
function clearInputError(eventOrInput) {
  const input = eventOrInput.target || eventOrInput;
  input.style.borderColor = '#e0e0e0';
  input.style.boxShadow = '';

  const errorDisplay = input.nextElementSibling;
  if (errorDisplay && errorDisplay.classList.contains('error-message')) {
    errorDisplay.remove();
  }
}

/**
 * Set up general page interactions
 */
function setupPageInteractions() {
  // Add fade-in animation to main content
  const main = document.querySelector('main');
  if (main) {
    main.classList.add('fade-in');
  }

  // Track page visibility
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
      log('Page hidden');
    } else {
      log('Page visible');
    }
  });

  // Log unhandled errors
  window.addEventListener('error', (event) => {
    console.error('Error caught:', event.error);
  });
}

/**
 * Fetch data with error handling
 * @param {string} url - URL to fetch from
 * @param {Object} options - Fetch options
 * @returns {Promise<any>} Response data
 */
async function fetchData(url, options = {}) {
  try {
    log('Fetching:', url);
    const response = await fetch(url, {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
      ...options,
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    const data = await response.json();
    log('Fetch successful:', url);
    return data;
  } catch (error) {
    console.error('Fetch error:', error);
    throw error;
  }
}

/**
 * Show toast notification
 * @param {string} message - Message to display
 * @param {string} type - Type: 'success', 'error', 'info'
 * @param {number} duration - Duration in ms
 */
function showToast(message, type = 'info', duration = 3000) {
  const toast = document.createElement('div');
  toast.textContent = message;
  toast.style.cssText = `
    position: fixed;
    bottom: 20px;
    right: 20px;
    padding: 15px 20px;
    border-radius: 5px;
    color: white;
    font-weight: 500;
    z-index: 9999;
    animation: slideUp 0.3s ease;
    max-width: 300px;
  `;

  // Set color based on type
  const colors = {
    success: '#27ae60',
    error: '#e74c3c',
    info: '#3498db',
  };
  toast.style.backgroundColor = colors[type] || colors.info;

  document.body.appendChild(toast);

  setTimeout(() => {
    toast.style.animation = 'fadeOut 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, duration);
}

/**
 * Debounce function to limit execution frequency
 * @param {Function} func - Function to debounce
 * @param {number} wait - Wait time in ms
 * @returns {Function} Debounced function
 */
function debounce(func, wait = 300) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}

/**
 * Throttle function to limit execution frequency
 * @param {Function} func - Function to throttle
 * @param {number} limit - Limit in ms
 * @returns {Function} Throttled function
 */
function throttle(func, limit = 300) {
  let inThrottle;
  return function (...args) {
    if (!inThrottle) {
      func.apply(this, args);
      inThrottle = true;
      setTimeout(() => (inThrottle = false), limit);
    }
  };
}

// Export functions for use in other modules (if using ES modules)
// export { fetchData, showToast, debounce, throttle, log };
